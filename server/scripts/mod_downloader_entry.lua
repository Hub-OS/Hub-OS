local ModDownloader = require("scripts/libs/mod_downloader")

local package_ids = {
  "BattleNetwork3.Canodumb",
  "BattleNetwork6.Mettaur",
  "BattleNetwork.Assets",
}

ModDownloader.maintain(package_ids)

Net:on("player_connect", function(event)
  -- preload mods on join
  for _, package_id in ipairs(package_ids) do
    Net.provide_package_for_player(event.player_id, ModDownloader.resolve_asset_path(package_id))
  end
end)
