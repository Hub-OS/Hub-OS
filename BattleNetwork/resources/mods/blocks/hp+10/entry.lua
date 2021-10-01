function package_init(block)
    block:declare_package_id("com.example.block.HP10")
    block:set_name("HP+10")
    block:as_program()
    block:set_description("+10 HP")
    block:set_color(1)
    block:set_shape({
        0, 0, 0, 0, 0,
        0, 1, 1, 1, 0,
        0, 0, 1, 0, 0,
        0, 0, 0, 0, 0,
        0, 0, 0, 0, 0
    })
end