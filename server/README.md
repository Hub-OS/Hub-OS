# Real PET Server

Scripting available through through Lua.

Support for more sources such as WASM/WASI (C++, Kotlin, etc) or JavaScript can be added by creating a Plugin Interface. For an example of how implement one, take a look at the existing LuaPluginInterface.

The Plugin Interface could also be used to build a Rust based script compiled directly into the server.

## Minimal Setup

- `areas` folder
  - `.tmx` map files, `default.tmx` required
  - See [Areas](#areas)
- `assets` folder
  - Tilesets, textures, audio, animations, packages
  - See [Assets](#assets)
- `scripts` folder
  - Lua files. Entry scripts are read from `scripts/*/main.lua` for script projects, and `scripts/*.lua` for single file scripts
  - See [Lua API](#lua-api)
- Server executable
  - See [Building the Project](#building-the-project) if you don't have a copy

Run the server through a command prompt or terminal and join on `127.0.0.1:8765`

Use `-h` to see more options and research port forwarding or server hosting if you want to share your server with others over the internet

## Assets

Types of assets:

- Texture (.png|.bmp)
- Audio (.ogg|.wav|.mid|.midi)
- Mod Packages (.zip)
- Text

Paths

- `/server`
  - Pseudo folder that represents files in memory
  - `/assets`
    - Generated at start from files in `./assets`.
    - `./assets/prog.png` can be referenced with `/server/assets/prog.png`
  - `/players`
    - Stores avatar files sent from players (5 MiB limit)
    - Textures are stored as `[id].texture`, and animations are stored as `[id].animation`
  - `/maps`
    - Generated from areas and updated every tick.
    - Stored as `[area id].txt`
- Paths not starting with `/server` are treated as paths to client files. Files of interest are available in `assets/`
- PATHS ARE CASE SENSITIVE OUT OF WINDOWS, also avoid `\` as that's Windows specific

## Areas

Maps for areas are stored in `./areas`. The first area players will see is `default.tmx` (required).

- [Suggested Settings](#suggested-settings)
- [Custom Properties](#custom-properties)
- [Object and Tile Types](#object-and-tile-types)

### Suggested Settings

Editor:

- Fine grid divisions: 2 (Edit -> Preferences -> Interface)
- Snap To Fine Grid (View -> Snapping)
  - When working with Object Layer
- Snap To Pixels (View -> Snapping)
  - When working with Collision shapes

Map:

- Tile Width: 64
- Tile Height: 32
- Tile Layer Format: CSV (required)
- Create map in assets

Tilesets:

- Type: Based on Tileset Image (other types are not currently supported)
- Object Alignment:
  - Top - For tile objects stuck to the floor such as warps
    - Set drawing offset to 0,0
  - Bottom - For tile objects that act as a wall
- Place in a Tile Layer to tune drawing offset

Layers:

- Horizontal Offset: 0
- Vertical Offset: (number of layers below this one) \* -16

### Custom properties

Map:

- Name: string
  - Area name to display in the PersonalMenu
- Song: string
  - Path to ogg file
- Background Texture: string
  - Path to background image file
- Background Animation: string
  - Path to background .animation file
  - One animation state "BG"
    - First frame of this animation determines background repetition
  - Excluding this will use texture size for background repetition
- Background Vel X: float
- Background Vel Y: float
- Background Parallax: float
- Foreground Texture: string
  - Path to foreground image file
- Foreground Animation: string
  - Path to foreground .animation file
  - One animation state "BG"
    - First frame of this animation determines foreground repetition
  - Excluding this will use texture size for foreground repetition
- Foreground Vel X: float
- Foreground Vel Y: float
- Foreground Parallax: float

Tiles:

- Shadow: string
  - "Always"
  - "Never"
  - Unset - Automatic

### Object and Tile Classes

Classes are used to denote special tiles or objects understood by the client.

- Warps
  - [Home Warp](#home-warp)
  - [Position Warp](#position-warp)
  - [Server Warp](#server-warp)
  - [Custom Server Warp](#custom-server-warp)
  - [Custom Warp](#custom-warp)
- Movement
  - [Stairs](#stairs)
  - [Conveyor](#conveyor)
  - [Ice](#ice)
  - [Treadmill](#treadmill)
- Plain Markers
  - [Board](#board)
  - [Shop](#shop)
  - [Arrow](#arrow)
  - [Invisible](#invisible)

#### Home Warp

- Tile Objects only
- Visible in minimap
- Players will be warped home if colliding with the warp
- Custom properties:
  - Direction: string
    - Left
    - Right
    - Up
    - Down
    - Up Left
    - Up Right
    - Down Left
    - Down Right

#### Position Warp

- Tile Objects only
- Visible in minimap
- Players will be warped to the set position if colliding with the warp
- Custom properties:
  - X: float
  - Y: float
  - Z: float
  - Direction: string
    - Left
    - Right
    - Up
    - Down
    - Up Left
    - Up Right
    - Down Left
    - Down Right

#### Server Warp

- Tile Objects only
- Visible in minimap
- Players will be transferred to a different server if colliding with the warp
- Custom properties:
  - Address: string
  - Data: string
    - Custom data to pass to the other server
    - Can be read through handle_player_request on the other server
    - Try to keep it short! Long data strings may get ignored
  - Direction: string
    - Left
    - Right
    - Up
    - Down
    - Up Left
    - Up Right
    - Down Left
    - Down Right

#### Custom Server Warp

- Tile Objects only
- Visible in minimap
- Players will be warped out if colliding with the warp, the result of the warp can be resolved in handle_custom_warp
- Custom Properties:
  - Direction: string
    - Left
    - Right
    - Up
    - Down
    - Up Left
    - Up Right
    - Down Left
    - Down Right

#### Custom Warp

- Tile Objects only
- Visible in minimap
- Players will be warped out if colliding with the warp, the result of the warp can be resolved in handle_custom_warp
  - Direction: string
    - Left
    - Right
    - Up
    - Down
    - Up Left
    - Up Right
    - Down Left
    - Down Right

#### Stairs

- Tiles only
- Visible in minimap
- Allows players to walk up or down a layer
- Makes tile directly above become treated as a hole
- Custom properties:

  - Direction: string
    - Marks the direction the player will travel up
    - Up Left
    - Up Right
    - Down Left
    - Down Right

#### Conveyor

- Tiles only
- Visible in minimap
- Custom properties:

  - Direction: string
    - Marks the direction the player will travel
    - Up Left
    - Up Right
    - Down Left
    - Down Right
  - Speed: number? (Tiles per second, default: 6)
  - Sound Effect: string

#### Ice

- Tiles only
- Custom properties:

  - Speed: number? (Tiles per second, default: 6)
  - Sound Effect: string

#### Treadmill

- Tiles only
- Custom properties:

  - Direction: string
    - Marks the direction the player will travel
    - Up Left
    - Up Right
    - Down Left
    - Down Right
  - Speed: number? (Tiles per second, default: 1.875)

#### Board

- Tile Objects only
- Visible in minimap

#### Shop

- Tile Objects only
- Visible in minimap

#### Arrow

- Tiles only
- Visible in minimap
- Custom properties:

  - Direction: string
    - Up Left
    - Up Right
    - Down Left
    - Down Right

#### Invisible

- Tiles only
- Hides the tile from players, great for invisible pathways

## Lua API

Commented functions are in development and require changes to the client (specified below).

- [Net Events](#net-events)
- [Net API](#net-api)
  - [Area API](#area-api)
  - [Bot API](#bot-api)
  - [Player API](#player-api)
  - [Widget API](#widget-api)
  - [Player Data API](#player-data-api)
  - [Asset API](#asset-api)
  - [Update Synchronization API](#update-synchronization-api)
- [Async API](#async-api)
- [Asyncified Net API](#asyncified-net-api)
- [Event Emitters](#event-emitters)

### Net Events

```lua
Net:on("tick", function(event)
  -- { delta_time: number (seconds) }
  print(event.delta_time)
end)

Net:on("authorization", function(event)
  -- a player on another server needs to be authenticated with this server
  -- the host for the other server is provided with the event for custom response / implementation
  -- do NOT share identity with other servers, use data for a temporary link between identities without sharing the identity
  -- { identity: string, host: string, data: string }
  print(event.identity, event.host, event.data)
end)

Net:on("player_request", function(event)
  -- player requests connection to server (only transfers and kicks should occur here)
  -- { player_id: string, data: string }
  print(event.player_id, event.data)
end)

Net:on("player_connect", function(event)
  -- player connects to the server (good place to setup while the player is loading)
  -- { player_id: string }
  print(event.player_id)
end)

Net:on("player_join", function(event)
  -- player enters their first area after connecting
  -- { player_id: string }
  print(event.player_id)
end)

Net:on("player_area_transfer", function(event)
  -- player changes area
  -- { player_id: string }
  print(event.player_id)
end)

Net:on("player_disconnect", function(event)
  -- the player is invalid after this function excecutes
  -- { player_id: string }
  print(event.player_id)
end)

Net:on("player_move", function(event)
  -- Net.get_player_position(event.player_id) will report the old position
  -- { player_id: string, x: number, y: number, z: number }
  print(event.player_id, event.x, event.y, event.z)
end)

Net:on("player_boost", function(event)
  -- base_health and max_health will be updated on the player before this function executes
  -- { player_id: string, blocks: { id: string }[] }
  print(event.player_id, event)
end)

Net:on("player_avatar_change", function(event)
  -- health, base_health, max_health, and element will be updated on the player before this function executes
  -- prevent_default blocks texture and animation change
  -- { player_id: string, texture_path: string, animation_path: string, prevent_default: Function }
  print(event.player_id, event)
end)

Net:on("player_emote", function(event)
  -- { player_id: string, emote: number, prevent_default: Function }
  print(event.player_id, event.emote)
end)

Net:on("custom_warp", function(event)
  -- player warped out by a "Custom Warp" or "Custom Server Warp"
  -- { player_id: string, object_id: number }
  print(event.player_id, event.object_id)
end)

Net:on("object_interaction", function(event)
  -- { player_id: string, object_id: number, button: number }
  print(event.player_id, event.object_id, event.button)
end)

Net:on("actor_interaction", function(event)
  -- { player_id: string, actor_id: string, button: number }
  -- actor_id is a player or bot id
  print(event.player_id, event.actor_id, event.button)
end)

Net:on("tile_interaction", function(event)
  -- { player_id: string, x: number, y: number, z: number, button: number }
  print(event.player_id, event.x, event.y, event.z, event.button)
end)

Net:on("textbox_response", function(event)
  -- { player_id: string, response: number | string }
  print(event.player_id, event.response)
end)

Net:on("board_open", function(event)
  -- deprecated
  print(event.player_id)
end)

Net:on("board_close", function(event)
  -- { player_id: string }
  print(event.player_id)
end)

Net:on("post_request", function(event)
  -- board post request for infinite scroll (UI has exhausted posts)
  -- { player_id: string }
  print(event.player_id)
end)

Net:on("post_selection", function(event)
  -- { player_id: string, post_id: string }
  print(event.player_id, event.post_id)
end)

Net:on("shop_close", function(event)
  -- { player_id: string }
  print(event.player_id)
end)

Net:on("shop_purchase", function(event)
  -- { player_id: string, item_name: string }
  print(event.player_id, event.item_name)
end)

Net:on("battle_results", function(event)
  -- { player_id: string, health: number, score: number, time: number, ran: bool, emotion: number, turns: number, enemies: { id: String, health: number }[] } }
  print(event.player_id, event.health, event.time, event.ran, event.emotion, event.turns, event.enemies)
end)

Net:on("server_message", function(event)
  -- { host: string, data: string }
  print(event.host, event.data)
end)
```

### Net API

Interactions with the cyberworld are performed through functions attached to a global table called `Net`. The APIs defined below are those functions categorized by what they affect.

#### Area API

```Lua
-- area_id is the filename without extension
-- ./assets/my_area.tmx would be my_area

Net.list_areas() -- area_id[]
-- Net.create_area(new_area_id)
Net.update_area(area_id, map_string)
Net.clone_area(area_id, new_area_id)
Net.remove_area(area_id)
Net.map_to_string(area_id)
Net.get_width(area_id)
Net.get_height(area_id)
Net.get_layer_count(area_id)
Net.get_tile_width(area_id)
Net.get_tile_height(area_id)
Net.get_area_custom_properties(area_id)
Net.get_area_custom_property(area_id, name)
Net.set_area_custom_property(area_id, name, value)
Net.get_area_name(area_id)
Net.set_area_name(area_id)
Net.get_song(area_id) -- song_path
Net.set_song(area_id, song_path)
Net.get_background(area_id) -- { texture_path, animation_path }
Net.get_background_velocity(area_id) -- { x, y }
Net.get_background_parallax(area_id) -- number
Net.set_background(area_id, texture_path, animation_path?, vel_x?, vel_y?, parallax?)
Net.get_foreground(area_id) -- { texture_path, animation_path }
Net.get_foreground_velocity(area_id) -- { x, y }
Net.get_foreground_parallax(area_id) -- number
Net.set_foreground(area_id, texture_path, animation_path?, vel_x?, vel_y?, parallax?)
Net.get_spawn_position(area_id) -- { x, y, z }
Net.set_spawn_position(area_id, x, y, z)
Net.get_spawn_direction(area_id)
Net.set_spawn_direction(area_id, direction)
Net.list_tilesets(area_id) -- tileset_path[]
Net.get_tileset(area_id, tileset_path) -- { path, first_gid }?
Net.get_tileset_for_tile(area_id, tile_gid) -- { path, first_gid }?
Net.get_tile(area_id, x, y, z) -- { gid, flipped_horizontally, flipped_vertically, rotated }
Net.set_tile(area_id, x, y, z, tile_gid, flip_h?, flip_v?, rotate?)
Net.provide_asset(area_id, path)
Net.play_sound(area_id, path)
```

#### Object API

```lua
Net.list_objects(area_id) -- object_id[]
Net.get_object_by_id(area_id, object_id) -- { id, name, class, type, visible, x, y, z, width, height, rotation, data, custom_properties }?
Net.get_object_by_name(area_id, name) -- { id, name, class, visible, x, y, z, width, height, rotation, data, custom_properties }?
Net.create_object(area_id, { name?, type?, visible?, x?, y?, z?, width?, height?, rotation?, data, custom_properties? }) -- object_id
Net.remove_object(area_id, object_id)
Net.set_object_name(area_id, object_id, name)
Net.set_object_class(area_id, object_id, class)
Net.set_object_type(area_id, object_id, type) -- deprecated
Net.set_object_custom_property(area_id, object_id, name, value)
Net.resize_object(area_id, object_id, width, height)
Net.set_object_rotation(area_id, object_id, rotation)
Net.set_object_visibility(area_id, object_id, visibility)
Net.move_object(area_id, object_id, x, y, layer)
Net.set_object_data(area_id, object_id, data)

-- possible values for data:
{
  type = "point" | "rect" | "ellipse"
}

{
  type = "polygon" | "polyline"
  points = { x, y }[],
}

{
  type = "tile",
  gid, -- int
  flipped_horizontally?, -- bool
  flipped_vertically?, -- bool
  rotated?, -- always false
}
```

#### Bot API

```lua
Net.list_bots(area_id) -- bot_id[]
Net.create_bot(bot_id, { name?, area_id?, warp_in?, texture_path?, animation_path?, animation?, x?, y?, z?, direction?, solid? })
Net.create_bot({ name?, area_id?, warp_in?, texture_path?, animation_path?, animation?, x?, y?, z?, direction?, solid? }) -- bot_id
Net.is_bot(bot_id)
Net.remove_bot(bot_id, warp_out?)
Net.get_bot_area(bot_id) -- area_id
Net.get_bot_name(bot_id) -- name
Net.set_bot_name(bot_id, name)
Net.get_bot_direction(bot_id)
Net.set_bot_direction(bot_id, direction)
Net.animate_bot_properties(bot_id, keyframes) -- unstable
Net.get_bot_position(bot_id) -- { x, y, z }
Net.move_bot(bot_id, x, y, z)
-- Net.set_bot_solid(bot_id, solid)
Net.set_bot_avatar(bot_id, texture_path, animation_path)
Net.set_bot_emote(bot_id, emote_id, use_custom_emotes?)
Net.set_bot_minimap_color(bot_id, color) -- color = { r: 0-255, g: 0-255, b: 0-255, a?: 0-255 }
Net.animate_bot(bot_id, state_name, loop?)
Net.transfer_bot(bot_id, area_id, warp_in?, x?, y?, z?)

-- keyframes:
{
  properties: {
    property: "Animation" | "Animation Speed" | "X" | "Y" | "Z" | "ScaleX" | "ScaleY" | "Rotation" | "Direction" | "Sound Effect" | "Sound Effect Loop",
    ease?: "Linear" | "In" | "Out" | "InOut" | "Floor",
    value: number | string
  }[],
  duration: number
}[]
```

#### Player API

```lua
Net.list_players(area_id) -- player_id[]
Net.is_player(player_id)
Net.get_player_area(player_id) -- area_id
Net.get_player_ip(player_id) -- address
Net.get_player_name(player_id) -- name
Net.set_player_name(player_id, name)
Net.get_player_direction(player_id)
Net.get_player_position(player_id) -- { x, y, z }
Net.get_player_mugshot(player_id) -- { texture_path, animation_path }
Net.get_player_avatar(player_id) -- { texture_path, animation_path }
Net.set_player_avatar(player_id, texture_path, animation_path)
Net.get_player_avatar_name(player_id) -- name
Net.set_player_emote(player_id, emote_id, use_custom_emotes?)
Net.exclusive_player_emote(player_id, emoter_id, emote_id, use_custom_emotes?)
Net.set_player_minimap_color(player_id, color) -- color = { r: 0-255, g: 0-255, b: 0-255, a?: 0-255 }
Net.animate_player(player_id, state_name, loop?)
Net.animate_player_properties(player_id, keyframes) -- unstable
Net.is_player_battling(player_id)
Net.is_player_busy(player_id)
Net.provide_asset_for_player(player_id, path)
Net.play_sound_for_player(player_id, path)
Net.exclude_object_for_player(player_id, object_id)
Net.include_object_for_player(player_id, object_id)
Net.exclude_actor_for_player(player_id, actor_id)
Net.include_actor_for_player(player_id, actor_id)
Net.enable_camera_controls(player_id, range_x?, range_y?) -- range is in pixels
Net.enable_camera_zoom(player_id)
Net.disable_camera_zoom(player_id)
Net.move_player_camera(player_id, x, y, z, holdTimeInSeconds?)
Net.fade_player_camera(player_id, color, durationInSeconds) -- color = { r: 0-255, g: 0-255, b: 0-255, a?: 0-255 }
Net.slide_player_camera(player_id, x, y, z, durationInSeconds)
Net.shake_player_camera(player_id, strength, durationInSeconds)
Net.track_with_player_camera(player_id, actor_id)
Net.unlock_player_camera(player_id)
Net.is_player_input_locked(player_id)
Net.lock_player_input(player_id)
Net.unlock_player_input(player_id)
Net.teleport_player(player_id, warp, x, y, z, direction?)
Net.offer_package(player_id, package_path)
Net.set_mod_whitelist_for_player(player_id, whitelist_path) -- whitelist has this format: `[md5] [package_id]\n`
Net.set_mod_blacklist_for_player(player_id, blacklist_path) -- blacklist has this format: `[md5] [package_id]\n`
Net.initiate_encounter(player_id, package_path, data?) -- data is a table, read as second param in package_build for the encounter package
Net.initiate_pvp(player_1_id, player_2_id, package_path?, data?)
Net.initiate_netplay(player_ids, package_path?, data?)
Net.transfer_player(player_id, area_id, warp_in?, x?, y?, z?, direction?)
Net.transfer_server(player_id, address, warp_out?, data?) -- data = string
Net.request_authorization(player_id, address, data?)
Net.kick_player(player_id, reason, warp_out?)
```

#### Widget API

```lua
Net.is_player_in_widget(player_id)
Net.is_player_shopping(player_id)
Net.message_player(player_id, message, mug_texture_path?, mug_animation_path?)
Net.question_player(player_id, question, mug_texture_path?, mug_animation_path?)
Net.quiz_player(player_id, option_a?, option_b?, option_c?, mug_texture_path?, mug_animation_path?)
Net.prompt_player(player_id, character_limit?, default_text?)

-- color = { r: 0-255, g: 0-255, b: 0-255 }, posts = { id: string, read: bool?, title: string?, author: string? }[]
-- returns EventEmitter, re-emits post_selection, post_request, board_close
Net.open_board(player_id, board_name, color, posts, open_instantly?)
Net.prepend_posts(player_id, posts, post_id?) -- unstable, issues arise when multiple scripts create boards at the same time
Net.append_posts(player_id, posts, post_id?) -- unstable, issues arise when multiple scripts create boards at the same time
Net.remove_post(player_id, post_id) -- unstable, issues arise when multiple scripts create boards at the same time
Net.close_bbs(player_id)

-- items = { name: string, description: string, price: number }[]
-- returns EventEmitter, re-emits shop_purchase, shop_close
Net.open_shop(player_id, items, mug_texture_path?, mug_animation_path?)
```

#### Player Data API

```lua
Net.get_player_secret(player_id) -- the secret identifier for this player. similar to a password, do not share
Net.get_player_element(player_id) -- string
Net.get_player_health(player_id)
Net.set_player_health(player_id, health)
Net.get_player_base_health(player_id)
Net.set_player_base_health(player_id, health)
Net.get_player_max_health(player_id)
Net.get_player_emotion(player_id)
Net.set_player_emotion(player_id, emotion)
Net.get_player_money(player_id)
Net.set_player_money(player_id, money)
Net.get_player_items(player_id) -- string[]
Net.give_player_item(player_id, item_id)
Net.remove_player_item(player_id, item_id)
Net.player_has_item(player_id, item_id)

Net.create_item(item_id, { name, description })
Net.get_item_name(item_id)
Net.get_item_description(item_id)
```

#### Asset API

```Lua
Net.update_asset(server_path, content)
Net.remove_asset(server_path)
Net.has_asset(server_path)
Net.get_asset_type(server_path)
Net.get_asset_size(server_path)
```

#### Update Synchronization API

```Lua
Net.synchronize(function()
  -- Net updates here will wait to be processed on clients when every update is received
end)

-- automatically called by the above function, be careful when using these directly
Net.request_update_synchronization()
Net.request_disable_update_synchronization()
```

### Async API

If you want to use IO while players are connected, you'll want to use the Async API to prevent server hiccups.
Note: paths in this section use system paths and not asset paths.

```Lua
-- promise objects returned by most async functions
promise.and_then(function(value))

Async.await(async_iterator) -- iterator
Async.await(promise) -- value -- for coroutines
Async.await_all(promises) -- values[] -- for coroutines
Async.promisify(coroutine) -- promise
Async.create_promise(function(resolve)) -- promise -- resolve = function(value)
Async.request(url, { method?, headers?, body? }?) -- promise, value = { status, headers, body }?
Async.download(path, url, { method?, headers?, body? }?) -- promise, value = bool
Async.read_file(path) -- promise, value = string
Async.write_file(path, content) -- promise, value = bool
Async.poll_server(address) -- promise, value = { }?
Async.message_server(address, data) -- you will not know if this succeeds, the other server will need to reply
Async.sleep(duration) -- promise, value = nil
```

### Asyncified Net API

Async alternatives to some Net API functions. Promises return nil if the user disconnects.

```lua
Async.message_player(player_id, message, mug_texture_path?, mug_animation_path?) -- promise, value = number?
Async.question_player(player_id, question, mug_texture_path?, mug_animation_path?) -- promise, value = number?
Async.quiz_player(player_id, option_a?, option_b?, option_c?, mug_texture_path?, mug_animation_path?) -- promise, value = number?
Async.prompt_player(player_id, character_limit?, default_text?) -- promise, value = string?
Async.initiate_encounter(player_id, package_path, data?) -- promise, value = { player_id: string, health: number, score: number, time: number, ran: bool, emotion: number, turns: number, enemies: { id: String, health: number }[] } }
Async.initiate_pvp(player_1_id, player_2_id, package_path?, data?) -- promise, value = { player_id: string, health: number, score: number, time: number, ran: bool, emotion: number, turns: number, enemies: { id: String, health: number }
Async.initiate_netplay(player_ids, package_path?, data?) -- promise, value = { player_id: string, health: number, score: number, time: number, ran: bool, emotion: number, turns: number, enemies: { id: String, health: number }[] } }[] } }
```

### Event Emitters

```lua
local emitter = Net.EventEmitter.new()
emitter:emit(event_name, ...)
emitter:on(event_name, function(...))
emitter:once(event_name, function(...))
emitter:on_any(function(event_name, ...))
emitter:on_any_once(function(event_name, ...))
emitter:remove_listener(event_name, callback)
emitter:remove_on_any_listener(callback)
emitter:async_iter(event_name) -- iterator that returns promises, value = ...
emitter:async_iter_all(event_name) -- iterator that returns promises, value = event_name, ...
emitter:destroy() -- allows async iterators to complete
```

### Lua STD Changes

`print` and `tostring` will display tables.

`printerr` will output red text to stdout.

## Building the Project

Windows requires for building lua [MSVC++](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#visual-studio-2015-2017-2019-and-2022)

This project is built with Rust, so after installing Cargo, you can compile and run the project with `cargo run`.

For a standalone executable you can run `cargo build --release` and copy just the executable from `target/build`

If you are interested in understanding the source before making changes, check out the [achitecture document](./ARCHITECTURE.md).

### Distributing

Install cargo-about: `cargo install cargo-about`

Run `cargo run --bin create_distributable`, a folder named `dist` will be created.
