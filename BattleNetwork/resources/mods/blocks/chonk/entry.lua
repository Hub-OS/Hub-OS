function package_init(block)
    block:declare_package_id("com.example.block.Chonk")
    block:set_name("Chonk")
    block:set_description("Big block\nbugs")
    block:set_color(Blocks.Green)
    block:set_shape({
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 0, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1
    })
end