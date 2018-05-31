    counter = 0
    request = function()
       mypath = "5KB.txt";
       local file = io.open(mypath, "r");
       assert(file);
       local body = file:read("*a");      -- 读取所有内容
       file:close();
       wrk.method = "PUT"
       wrk.body = body
       path = "/public/test-" .. mypath .. "-" .. counter
       wrk.headers["X-Counter"] = counter
       counter = counter + 1
       return wrk.format(nil, path)
    end
    done = function(summary, latency, requests)
       io.write("------------------------------\n")
       for _, p in pairs({ 50, 60, 90, 95, 99, 99.999 }) do
          n = latency:percentile(p)
          io.write(string.format("%g%%, %d ms\n", p, n/1000.0))
       end
    end
