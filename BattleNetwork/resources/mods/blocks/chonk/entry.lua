function package_init(block)
    block:declare_package_id("com.example.block.Chonk")
    block:set_name("Chonk")
    block:set_description("Big block bugs")
    block:set_color(2)
    block:set_shape({
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 0, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1
    })
end