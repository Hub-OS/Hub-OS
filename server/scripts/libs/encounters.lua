local lib = {
    BIG_TILE_MOVEMENT = 1, -- users can't walk/run this far without help from the server
    CHECK_DISTANCE = 2.0,  -- at what distance to test for encounters
}

local player_trackers = {}
local area_tables = {}

function lib.track_player(player_id)
    local area_id = Net.get_player_area(player_id)
    local area_table = area_tables[area_id]

    player_trackers[player_id] = {
        last_pos = Net.get_player_position(player_id),
        area = area_id,
        total_dist = 0,
        last_test = ((area_table and area_table.min_travel) or lib.CHECK_DISTANCE) - lib.CHECK_DISTANCE
    }
end

function lib.drop_player(player_id)
    player_trackers[player_id] = nil
end

function lib.setup(area_tables)
    for area_id, area_table in pairs(area_tables) do
        lib.setup_area(area_id, area_table)
    end
end

function lib.setup_area(area_id, area_table)
    local weighted_sum = 0

    for _, encounter in ipairs(area_table.encounters) do
        weighted_sum = weighted_sum + encounter.weight

        if area_table.preload then
            Net.provide_asset(area_id, encounter.asset_path)
        end
    end

    if weighted_sum ~= 1.0 then
        -- we must normalize
        for _, encounter in ipairs(area_table.encounters) do
            encounter.weight = encounter.weight / weighted_sum
        end
    end

    area_tables[area_id] = area_table
end

function lib.handle_player_move(player_id, x, y, z)
    if Net.is_player_busy(player_id) then
        return
    end

    local area_id = Net.get_player_area(player_id)
    local player_tracker = player_trackers[player_id]

    if area_id ~= player_tracker.area then
        -- changed area, reset
        lib.track_player(player_id)
        return
    end

    local area_table = area_tables[area_id]

    if not area_table or #area_table.encounters == 0 then
        -- no encounters for this area
        return
    end

    local deltax = x - player_tracker.last_pos.x
    local deltay = y - player_tracker.last_pos.y

    local screenx = deltax - deltay
    local screeny = (deltax + deltay) * 0.5
    local delta_dist = math.sqrt(screenx * screenx + screeny * screeny)

    if delta_dist > lib.BIG_TILE_MOVEMENT then
        -- teleported
        return
    end

    player_tracker.total_dist = player_tracker.total_dist + delta_dist
    player_tracker.last_pos.x = x
    player_tracker.last_pos.y = y

    if player_tracker.total_dist - player_tracker.last_test < lib.CHECK_DISTANCE then
        return
    end

    player_tracker.last_test = player_tracker.last_test + lib.CHECK_DISTANCE

    -- calculate chance
    local chance = area_table.chance

    if type(chance) == "function" then
        chance = chance(player_tracker.total_dist)
    end

    if math.random() > chance then
        return
    end

    local crawler = math.random()

    for i, encounter in ipairs(area_table.encounters) do
        crawler = crawler - encounter.weight

        if crawler <= 0 or i == #area_table.encounters then
            -- send encounter
            Net.initiate_encounter(player_id, encounter.asset_path)
            -- reset tracking
            lib.track_player(player_id)
            break
        end
    end
end

function lib.initiate_direct_encounter(player_id, asset_path)
    lib.track_player(player_id)
    Net.initiate_encounter(player_id, asset_path)
end

return lib

--[[
    -- Setup example below

    local encounter_table = {}

    encounter_table["central_area_1"] = {
        chance = .05, -- chance out of 1, tested at every tile's worth of movement
        min_travel = 2, -- optional, 2 tiles are free to walk after each encounter
        preload = true, -- optional, preloads every encounter when the player joins the area
        encounters = {
            {
                asset_path = "/server/assets/basic_mob1.zip",
                weight = 0.1
            },
            {
                asset_path = "/server/assets/basic_mob2.zip",
                weight = 0.5
            },
            {
                asset_path = "/server/assets/basic_mob3.zip",
                weight = 0.01
            },
        }
    }

    encounter_table["central_area_2"] = {
        chance = function(distance)
            -- https://pastebin.com/vXbWiKAc
            local index = math.floor(distance / Encounters.CHECK_DISTANCE)
            local chance_curve = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 8, 10, 12 }

            local chance = chance_curve[index] or chance_curve[#chance_curve]
            return chance / 32
        end,
        encounters = { ... etc ... }
    }

    Encounters:setup(encounter_table)
]]
