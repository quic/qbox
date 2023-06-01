
-- ** Convenience functions **
 function file_exists(name)
    local f=io.open(name,"r")
    if f~=nil then io.close(f) return true else return false end
 end

 function valid_file(file)
    local f = io.open(file, "r")
    if (f == nil)
    then
        print ("ERROR:")
        print (file.." Not found.")
        os.exit(1)
    end
    io.close(f);
    return file
 end
 function tableMerge(t1, t2)
    for k,v in pairs(t2) do
        if type(v) == "table" then
            if type(t1[k] or false) == "table" then
                tableMerge(t1[k] or {}, t2[k] or {})
            else
                t1[k] = v
            end
        else
            t1[k] = v
        end
    end
    return t1
end
function tableJoin(t1,t2)
    for i=1,#t2 do
        t1[#t1+1] = t2[i]
    end
    return t1
end

-- Override assert() as the default leads to odd errors in qqvp
function assert(exp, msg)
  if msg == nil then
    msg = "Assertion failed!"
  end
  if not exp then
    print(msg)
    print(debug.traceback())
    os.exit(1)
  end
end
-- ** End of convenience functions **

