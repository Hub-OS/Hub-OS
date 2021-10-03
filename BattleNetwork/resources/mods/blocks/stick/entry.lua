function package_init(block)
    block:declare_package_id("com.example.block.Stick")
    block:set_name("Stick")
    block:as_program()
    block:set_description("for crafting\npickaxe")
    block:set_color(3)
    block:set_shape({
        0, 0, 0, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 0, 0, 0
    })
end