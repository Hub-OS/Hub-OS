local function create_test(message)
  return function(a, b, c)
    local matches = a == 1 and b == 2 and c == 3

    if not matches then
      error(message)
    end
  end
end

local promise = Async.create_promise(function(resolve) resolve(1, 2, 3) end)

promise.and_then(create_test("multi value resolve failed"))

local coroutine_promise = Async.promisify(coroutine.create(function()
  local a, b, c = Async.await(promise)
  create_test("multi value await failed")(a, b, c)

  return 1, 2, 3
end))

coroutine_promise.and_then(create_test("multi value coroutine failed"))
