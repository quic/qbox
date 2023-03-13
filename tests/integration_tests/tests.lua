--
-- verbose 0 nothing
-- verbose 1 - final report
-- verbose 2 - events as they happen
--
verbose=2
csvfile="report.csv"

math.randomseed(1)
--    math.randomseed(os.time())

function merge(t1, t2)
   for k, v in pairs(t2) do
      if (type(v) == "table") and (type(t1[k] or false) == "table") then
         merge(t1[k], t2[k])
      else
         t1[k] = v
      end
   end
   return t1
end
--
-- components
--
--    function timed()  return math.random(1,10)  end
--    function waiting()  return math.random(10,100)  end
function timed()  return 1  end
function untimed()  return 0  end
function waiting()  return 11  end
function nowait()  return 0  end
function delay() return 17 end
function nodelay() return 0 end

-- this list could be automagically pre-built when the lua is loaded!
modules={"TLMClockedSlave","QemuLikeMaster", "MasterSimple", "Clock", "RealTimeClockLimiter"}
for  _, m in pairs(modules) do 
   _G[m]=function(o) return merge ({moduletype=m},o) end
end


function tlmslave(n, s_wait, s_delay)
   local v={};
   v["t"..n] = TLMClockedSlave
   {
      add_base=n,
      add_size=1,
      tlm_mode=true,
      ms_sleep=0,
      ns_wait=s_wait,
      ns_delay=s_delay
   }
   return v
end

    function qemu(sync, n, m_timing)
    local v={}
    v["q"..n] = QemuLikeMaster
    {   run_count=100000,
        send_to=n,
        sync_policy=sync,
        MIPS=100,
        iorate=7,
        ms_sleep=m_timing
    }
    return v
    end

    function master(n, m_timing)
    local v={}
    v["m"..n] = MasterSimple
    {
        run_count=-1,
        send_to=n,
        ms_sleep=m_timing
    }
    return v
    end

    function defaultrun() 
      return {
        ns_quantum=1000,
        ns_run=500000,
        rerun=7,
        verbose=0,
      }
    end


                
    function tt() 
       local tt=setmetatable(
          defaultrun(),
          {__add = function(tt,t2) return merge(tt,t2) end}
       )
      return tt
    end
            
--
-- all fnctions with the name test_... will be used to generate tests
--
--

function test_a_simple(sync)
    return    tt()+
    master(1, untimed()) +
    tlmslave(1,  waiting(), nodelay()) +
    {
      description="master, untimed, with wait and clock",
      c1 = Clock{ ns_wait = 10 },
--      verbose=2
    }
end

function test_a_simple_rt(sync)
    return    tt()+
    master(1, untimed()) +
    tlmslave(1,  nowait(), nodelay()) +
    {
      description="master, untimed, nowait, realtime",
      r1 = RealTimeClockLimiter{use=true},
      m1 = {ns_wait=1000000},
      ns_run = 500000000,
--      verbose=2
    }
end
    
    
function test_qemu(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, nowait(), nodelay()) +
    {
      description="Qemu untimed trivial slave",
      q1 = { run_count = 10 },
--      verbose=2
    }
end
function test_qemu_tbound(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, nowait(), nodelay()) +
    {
      description="Qemu untimed, trivial slave bound by time",
      ns_run = 300000,
--      verbose=2
    }
end
function test_qemu_sparse_txns(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, nowait(), nodelay()) +
    {
      description="Qemu untimed with sparse txns",
      q1 = {iorate=14000},
    }
end


function notest_qemu_wait(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, waiting(), nodelay()) +
    {
      description="Qemu untimed slave wait",
    }
end

function test_qemu_delay_slowsc(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, nowait(), delay()) +
    {
      description="Qemu (untimed) , slave wait, rapid clock",
      c1 = Clock{
          ns_wait=100,
          ms_sleep=1,
          record=false
      },
      q1 = { run_count = 10 },
    }
end


function test_qemu_delay(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, nowait(), delay()) +
    {
      description="Qemu (untimed) , slave wait, rapid clock",
      c1 = Clock{
          ns_wait=100,
          ms_sleep=1,
          record=false
      },
      q1 = { run_count = 10 },
    }
end



function test_qemu_timed_wait(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, waiting(), nodelay()) +
    {
      description="Qemu untimed, slave waits",
    }
end

function test_qemu_timed_wait_illegal(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, waiting(), nodelay()) +
    {
      description="Qemu untimed, slave waits, but doesn't update delay (illegal TLM)",
      t1 = { tlm_mode = false },
    }
end


function test_timed_qemu_timed_wait(sync)
    return tt()+
    qemu(sync, 1, timed()) +
    tlmslave(1, waiting(), nodelay()) +
    {
      description="Timed Qemu , slave waits",
      ns_run = 50000,  -- short run
    }
end

function test_qemu_bigwait(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, 3000, nodelay()) +
    {
      description="Qemu master, slave large wait (forcing systemc time forward, qemu can't catch up)",
--      verbose=2,
      ns_run=500000,
    }
end

function test_double_qemu(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, nowait(), nodelay()) +
    qemu(sync, 2, untimed()) +
    tlmslave(2, nowait(), nodelay()) +
    {
      description="Two Qemu masters, untimed no wait",
      ns_run=10000,
      ns_quantum=100,
--      verbose=2
    }
end

function notest_double_qemu_bigwait(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, 3000, nodelay()) +
    qemu(sync, 2, untimed()) +
    tlmslave(2, 3000, nodelay()) +
    {
      description="Two Qemu master, slave big wait",
    }
end

function test_double_timed_qemu_speed_test(sync)
    return tt()+
    qemu(sync, 1, timed()) +
    tlmslave(1, untimed(), nodelay()) +
    qemu(sync, 2, timed()) +
    tlmslave(2, untimed(), nodelay()) +
    {
      description="Two 'timed' Qemu masters, trivial slaves - should be parallel ",
      ns_quantum=100,
      ns_run=10000,
--      rerun=1,
--      verbose=2
    }
end
function test_double_qemu_txn_speed_test(sync)
    return tt()+
    qemu(sync, 1, untimed()) +
    tlmslave(1, untimed(), nodelay()) +
    qemu(sync, 2, untimed()) +
    tlmslave(2, untimed(), nodelay()) +
    {
      description="Two untimed Qemu masters, trivial slaves, many txns - should be parallel ",
      ns_quantum=100,
      ns_run=100000,
      q1={iorate=1},
      q2={iorate=1},
      rerun=1,
--      verbose=2
    }
end

function notest_timed_double_qemu_odd_waits(sync)
    return tt()+
    qemu(sync, 1, timed()) +
    tlmslave(1, waiting(), nodelay()) +
    qemu(sync, 2, timed()) +
    tlmslave(2, 3000, nodelay()) +
    {
      description="Two Qemu masters, two slaves, one with large wait (3/5 quantum)",
    }
end
    
function notest_double_qemu_master_slowsc(sync)
    return tt()+
    qemu(sync, 1, timed()) +
    tlmslave(1, nowait(), delay()) +
    qemu(sync, 2, timed()) +
    tlmslave(2, waiting(), nodelay()) +
    master(3, timed()) +
    tlmslave(3, waiting(), nodelay()) +
    {
      description="Two Qemu masters and a normal master and 2 slaves that wait, one using delays, and slow systemc",
      c1=Clock
      {
          ns_wait=50,
          ms_sleep=1,
          record=false
      },
    }
end


--    syncs={ "tlm2"}
--    syncs={"multithread-tlm2"}
--    syncs={"multithread"}
--    syncs={ "delayed"}
--    syncs={ "async-window" }
--    syncs={ "async-window", "synchronous" }
--    syncs={ "multithread", "multithread-tlm2"}
      syncs={ "tlm2", "multithread", "multithread-tlm2", "multithread-quantum"}
--    syncs={ "delayed", "synchronous", "async-window", "async-unconstrained", "tlm2_sp"}
--    syncs={ "tlm2", "multithread", "multithread-tlm2", "multithread-quantum", "delayed", "synchronous", "async-window", "async-unconstrained", "tlm2_sp"}
--    syncs={ "tlm2", "multithread", "multithread-tlm2", "multithread-quantum",  "synchronous", "async-window", "async-unconstrained", "tlm2_sp"}
--    syncs={ "tlm2", "multithread", "multithread-quantum",  "synchronous", "async-window", "async-unconstrained", "tlm2_sp"}

tests = {};
n=0;

function spairs(t)
  local i = 0
  local keys={}
  for k in pairs(t) do keys[#keys+1] = k end
  table.sort(keys)
  return function()
    i = i + 1
    if keys[i] then
      return keys[i], t[keys[i]]
    end
  end
end

allfns={ }
for key, item in pairs(_G) do
  if type(item) == "function" and string.sub(key,0,5)=="test_" then
    allfns[#allfns+1]=key
  end
end
for _,item in ipairs(allfns) do
  for k,sync in pairs(syncs) do
        name=string.sub(item,6)
        tests["t_"..name.."__"..sync]=_G[item](sync)
  end
end

