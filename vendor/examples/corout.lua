#!/usr/bin/lua
-- Messing with coroutines


co = coroutine.create(function ()
  for i=1,10 do
    print("hi", i)
    coroutine.yield()
  end
end)

print(co)
print(coroutine.status(co))
coroutine.resume(co)
print(coroutine.status(co))
coroutine.resume(co)
print(coroutine.status(co))
