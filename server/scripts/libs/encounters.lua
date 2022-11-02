local lib = {
    BIG_TILE_MOVEMENT = 1, -- users can't walk/run this far without help from the server
}

local player_trackers = {}
local table = {}

function lib.track_player(player_id)
    player_trackers[player_id] = {
        last_pos = Net.get_player_position(player_id),
        area = Net.get_player_area(player_id),
        amt = 0
    }
end

function lib.drop_player(player_id)
    player_trackers[player_id] = nil
end

function lib.setup(encounter_table)
    for area_id, area_table in pairs(encounter_table) do
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

    table[area_id] = area_table
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

    local area_table = table[area_id]

    if not area_table or #area_table.encounters == 0 then
        -- no encounters for this area
        return
    end

    local pos = {x = x, y = y}
    local amtx = math.abs(player_tracker.last_pos.x - pos.x)
    local amty = math.abs(player_tracker.last_pos.y - pos.y)

    if amtx + amty > lib.BIG_TILE_MOVEMENT then
        -- teleported
        return
    end

    local total_amt = player_tracker.amt + amtx + amty
    player_tracker.amt = total_amt
    player_tracker.last_pos = pos

    local required_travel_amt = area_table.min_travel

    if total_amt < required_travel_amt then
        return
    end

    if math.random() > area_table.chance then
        return
    end

    local crawler = math.random()

    for i, encounter in ipairs(area_table.encounters) do
        crawler = crawler - encounter.weight

        if crawler <= 0 or i == #area_table.encounters then
            lib.track_player(player_id)
            Net.initiate_encounter(player_id, encounter.asset_path)
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
        min_travel = 2, -- 2 tiles are free to walk after each encounter
        chance = .05, -- chance for each movement event after the minimum travel to cause an encounter,
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
        min_travel = 5, -- long breaks...
        chance = .01, -- less enemies...
        encounters = { ... etc ... }
    }

    Encounters:setup(encounter_table)
]]
