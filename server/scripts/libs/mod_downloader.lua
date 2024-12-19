local json = require("scripts/libs/json")

local ModDownloader = {
  REPO = "https://hubos.dev",
  FOLDER = "downloaded_mods"
}

---@param package_id_list string[]
function ModDownloader.maintain(package_id_list)
  local encoded_package_ids = {}

  for _, package_id in ipairs(package_id_list) do
    encoded_package_ids[#encoded_package_ids + 1] = Net.encode_uri_component(package_id)
  end

  local encoded_ids_string = table.concat(encoded_package_ids, "&id=")

  return Async.create_scope(function()
    local response = Async.await(Async.request(ModDownloader.REPO .. "/api/mods/hashes?id=" .. encoded_ids_string))

    if response.status ~= 200 then
      warn("[mod downloader] hash request failed: " .. response.body)
      return nil
    end

    local hash_map = {}

    for _, package_hash_result in ipairs(json.decode(response.body)) do
      hash_map[package_hash_result.id] = package_hash_result.hash
    end

    Async.await(Async.ensure_dir("assets/" .. ModDownloader.FOLDER))

    for _, package_id in ipairs(package_id_list) do
      local encoded_id = Net.encode_uri_component(package_id)
      local zip_path = "assets/" .. ModDownloader.FOLDER .. "/" .. encoded_id .. ".zip"
      local server_path = "/server/" .. zip_path

      local remote_hash = hash_map[package_id]
      local current_hash = Net.get_asset_hash(server_path)

      if not remote_hash then
        warn("[mod downloader] package repo could not find: " .. package_id)
      elseif remote_hash and current_hash ~= remote_hash then
        Async.await(Async.download(zip_path, ModDownloader.REPO .. "/api/mods/" .. encoded_id))

        local data = Async.await(Async.read_file(zip_path))
        Net.update_asset(server_path, data)
      end
    end

    return nil
  end)
end

---@param package_id_list string[]
function ModDownloader.download_once(package_id_list)
  return Async.create_scope(function()
    Async.await(Async.ensure_dir("assets/" .. ModDownloader.FOLDER))

    for _, package_id in ipairs(package_id_list) do
      local encoded_id = Net.encode_uri_component(package_id)
      local zip_path = "assets/" .. ModDownloader.FOLDER .. "/" .. encoded_id .. ".zip"
      local server_path = "/server/" .. zip_path

      if not Net.has_asset(server_path) then
        Async.await(Async.download(zip_path, ModDownloader.REPO .. "/api/mods/" .. encoded_id))

        local data = Async.await(Async.read_file(zip_path))
        Net.update_asset(server_path, data)
      end
    end

    return nil
  end)
end

---@param package_id string
function ModDownloader.resolve_asset_path(package_id)
  return "/server/" .. ModDownloader.resolve_file_path(package_id)
end

---@param package_id string
function ModDownloader.resolve_file_path(package_id)
  local encoded_id = Net.encode_uri_component(package_id)
  return "assets/" .. ModDownloader.FOLDER .. "/" .. encoded_id .. ".zip"
end

return ModDownloader
