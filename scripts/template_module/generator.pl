#!/usr/bin/perl

# e.g. goto : https://ipcatalog.qualcomm.com/swi/export?chip=347&version=14217
# select export as JSON


use Data::Dumper;
use File::Find;
use Path::Tiny qw(path);
use File::Path qw(make_path remove_tree);
use File::Copy::Recursive qw(fcopy rcopy dircopy fmove rmove dirmove);
use File::Basename;
use Cwd;
use File::Basename;
use Math::BigInt;
use JSON;
use Archive::Zip qw( :ERROR_CODES :CONSTANTS );

my $dir = getcwd;
my $sdir=path(dirname(__FILE__)."/init-template")->realpath;
use Encode qw(decode encode);


sub findModule(@_){
	my $node=shift(@_);
	my $name=shift(@_);

	if ($node->{NAME} eq $name or $node->{CC_INFO}->{COMPONENT} eq $name) {
		return $node;
	}
	my $mods=$node->{MODULES};
	foreach my $mod (@$mods) {
		if ($mod->{NAME} eq $name or $mod->{CC_INFO}->{COMPONENT} eq $name) {
			return $mod;
		}
	}
	die "Can't find $name";
}


sub makeAddrMap(@_){
	my $node=shift(@_);
	my $full=shift(@_);

	my $thismod;

	$node->{ADDRESS} and $thismod->{ADDRESS}=$node->{ADDRESS};
	$node->{OFFSET} and $thismod->{OFFSET}=$node->{OFFSET};	
	$node->{SIZE} and $thismod->{SIZE}=$node->{SIZE};
	$node->{NAME} and $thismod->{NAME}=$node->{NAME};
	$node->{MEMORY} and $thismod->{MEMORY}=$node->{MEMORY};

	my $mods=$node->{MODULES};
	my @modlist;
	foreach my $mod (@$mods) {
		if ($full==-1) {
			push @modlist, makeAddrMap($mod, $full);
		} else {
			if ($full>0) {
				push @modlist, makeAddrMap($mod, $full-1);			
			}
		}
	}
	my %cnt;
	for (@modlist) {
		$cnt{$_->{NAME}}++ and $_->{NAME}=$_->{NAME}."_".($cnt{$_->{NAME}} -1);
#		die "Duplicate ".$_->{NAME};
	}
	@modlist and $thismod->{MODULES}=\@modlist;
	my $regs=$node->{REGISTERS};
	if ($full==-1) {
		my @reglist;
		foreach my $reg (@$regs) {
			my %variants;
			if ($reg->{INDICES}) {
				my @ind_name, @ind_min, @ind_max, $indn=0;
				foreach my $ind (@{$reg->{INDICES}}) {
					$ind_name[$indn] = $ind->{STRING};
					$ind->{RANGE} =~ m/\[([0-9]+)\.\.([0-9]+)\]/;
					$ind_min[$indn]=$1;
					$ind_max[$indn]=$2;
					$ind_val[$indn]=$1;
					$indn++;
				}
				while(1) {
					my $name=$reg->{NAME};
					my $offset=$reg->{OFFSET};
					for ($i=0;$i<$indn;$i++) {
						$name =~ s/\%$ind_name[$i]\%/$ind_val[$i]/g;
						$offset =~ s/\%$ind_name[$i]\%/$ind_val[$i]/g;
					}
					$offset = sprintf("0x%X",eval($offset));
					$variants{$name}=$offset;
					for ($i=0;$i<$indn;$i++) {
						$ind_val[$i]++;
						if ($ind_val[$i]>$ind_max[$i]) {
							$ind_val[$i]=$ind_min[$i];
						}else {
							last;
						}
					}
					($i>=$indn) and last;
				}
			} else {
				$variants{$reg->{NAME}}=sprintf("0x%X",eval($reg->{OFFSET}));
			}
			foreach my $v (keys %variants) {
				my $r;
				$r->{NAME} = $v;
				$r->{ACCESS} = $reg->{ACCESS};
				$r->{OFFSET} = $variants{$v};
				$r->{RESETVALUE} = $reg->{RESETVALUE};
				$r->{BITWIDTH} = $reg->{BITWIDTH};
				$r->{FIELDS} = $reg->{FIELDS};
				push @reglist, $r;
			}
		}
		@reglist and $thismod->{REGISTERS}=\@reglist;
	} else {
		if (@$regs) {
			my $m;
			$m->{NAME}="reg_mem";
			$m->{SIZE}=$node->{SIZE};
			$m->{ADDRESS}=$node->{ADDRESS};
			$m->{OFFSET}=$node->{OFFSET};			
			push @{$thismod->{MEMORY}}, $m;
		}
	}
	return $thismod;
}


sub getmod(@_){
	my $jsonfile=shift(@_);
	my $search=shift(@_);

	my $data = $jsonfile->slurp;
	my $top  = decode_json $data;
	my $mod=$top;
	($top->{CHIP}) and $mod=$top->{CHIP};
	for (split /\./, $search) {
		$mod=findModule($mod,$_);
	}
	print "Found ".$mod->{NAME}. " Component ".$mod->{CC_INFO}->{COMPONENT}."\n";
	return $mod;
}


sub makeAddrMapZip(@_){
	my $jsonfile=shift(@_);
	my $outfile=shift(@_);
	my $search=shift(@_);
	my $full=shift(@_);

	my $mod = getmod($jsonfile, $search);

	my $zip = Archive::Zip->new();
	my $string_member = $zip->addString( encode("ascii",to_json(makeAddrMap($mod, $full))), (split /\./, $search)[-1].".json" );
	$string_member->desiredCompressionMethod( COMPRESSION_DEFLATED );
	$zip->writeToFileNamed($outfile);
}


sub jsonSplit(@_){
	my $node=shift(@_);
	my $zip=shift(@_);
	my $path=shift(@_);
	my $localpath = $path.$node->{NAME};

	my $mods=$node->{MODULES};
	my @modlist;
	foreach my $mod (@$mods) {
		push @modlist, jsonSplit($mod, $zip, "$localpath/");
	}
	@modlist and $node->{MODULES}=\@modlist;

	my $string_member = $zip->addString( encode("ascii",to_json($node)), "$localpath.json" );
	$string_member->desiredCompressionMethod( COMPRESSION_DEFLATED );

	return $node->{NAME};
}


sub makeJsonSplitZip(@_){
	my $jsonfile=shift(@_);
	my $outfile=shift(@_);

	my $mod = getmod($jsonfile);

	my $zip = Archive::Zip->new();
	jsonSplit($mod, $zip, "");
	$zip->writeToFileNamed($outfile);
}


sub usage($@) {
	die <<EOF
$_[0]
Usage $0 <source.json> [--init <module name>] [--top <destination file>] [--full <destination file>]

	The source.json file should be downloaded directly from ipcat (for the entire chip)

	--init construct a full module, in the current directory, from a template (including the ZIP file)
		$0 must be run in the target new module directory.
		The module name can either be the hierarhcical name of a module, or it's component name.
	--top output a top level zip file that does not contain register information, just hierarchical paths and addresses.
		The desitination zip file will be named \"<destination file>-modonly.zip\"
	--full output a top level zip file that contain all information including registers
		The desitination zip file will be named \"<destination file>-full.zip\"
	--split output the json file, split into a heirarchy of module files within the zip.
EOF
	  ;
}

($#ARGV!=2) and usage("$0 expects 3 arguments");

my $basejsonfile=$ARGV[0];
(-f $basejsonfile) or usage("$basejsonfile not found");

$basejsonfile = path($basejsonfile);

my $replace_str="";


sub replace {
	if (-f $File::Find::name) {
		my $file=$File::Find::name;
		my $sfile=path($file);
		$file =~ s/$sdir/$dir/;
		$file =~ s/_MODULE_/$replace_str/g;
		my $dfile=path($file);
		make_path(dirname($dfile));
		if (!-f $dfile) {
			my $data = $sfile->slurp_utf8;
			$data =~ s/_MODULE_/$replace_str/g;
			$dfile->spew_utf8( $data );
		} else {
			print "Wont overwrite $dfile\n";
		}
	}
}

if ($ARGV[1] eq "--init") {
	#
	#  -- Initialize a new module
	#
	my $mod=join('_',(split /\./, $ARGV[2]));#[-1];
	print "Initializing $mod in $dir\n";

	$replace_str=$mod;

	find( \&replace, "$sdir");
	makeAddrMapZip($basejsonfile, $dir."/src/quic/${mod}/${mod}.cc_config.zip", $ARGV[2], -1);


} elsif ($ARGV[1] eq "--top") {
	makeAddrMapZip($basejsonfile, $ARGV[2]."-modonly.zip", $ARGV[2], 1);
} elsif ($ARGV[1] eq "--full") {
	makeAddrMapZip($basejsonfile, $ARGV[2]."-full.zip", $ARGV[2], -1);
} elsif ($ARGV[1] eq "--split") {
	makeJsonSplitZip($basejsonfile, $ARGV[2]."-split.zip");
} else {
	usage("Unknown argument $ARGV[1]");
}
