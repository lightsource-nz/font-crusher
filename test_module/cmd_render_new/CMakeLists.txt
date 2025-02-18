# cmd_render_new: test package for basic functionality of command 'crush render new'
add_test(NAME cmd_render_new COMMAND "crush render new test_render")

set_property(TEST cmd_render_new PROPERTY ENV CRUSH_CONTEXT=)
