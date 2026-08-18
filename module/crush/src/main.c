#include <crush.h>
#include <module/mod_light_cli.h>
#include <module/mod_libcrush.h>
#include "crush_private.h"

#define CRUSH_ROOT_COMMAND_NAME                 "crush"
#define CRUSH_ROOT_COMMAND_DESCRIPTION          "default root command for the Font Crusher application"

#define CRUSH_VERSION_MAJOR                     1

static void crush_app_event(const struct light_module *mod, uint8_t event, void *arg);
static uint8_t crush_app_main(struct light_application *app);

Light_Application_Define(
        crush, crush_app_event, crush_app_main,
        &libcrush,
        &light_cli,
        &light_core
);

static struct light_cli_invocation_result do_cmd_crush(struct light_cli_invocation *invoke);
Light_Command_Define(cmd_crush, &root_command, CRUSH_ROOT_COMMAND_NAME, CRUSH_ROOT_COMMAND_DESCRIPTION, do_cmd_crush, 0, 0);

static void print_usage(void);
int main(int argc, char *argv[])
{
        light_framework_init();
        // crush is invoked from build systems, which decide whether a step succeeded from the
        // exit code and nothing else. this used to fall off the end of main() and so always
        // reported success -- a failed `crush font add` or a failed render left the build
        // green and quietly reusing whatever stale generated sources were already on disk
        uint8_t status = light_framework_run(argc, argv);
        return status == LF_STATUS_ERROR ? 1 : 0;
}
static void crush_app_event(const struct light_module *mod, uint8_t event_id, void *arg)
{
        switch (event_id)
        {
        case LF_EVENT_APP_LAUNCH:
                struct light_event_app_launch *event = (struct light_event_app_launch *)arg;
                light_info("Received LF_EVENT_APP_LAUNCH, argument count: %d", event->argc);
                break;
        case LF_EVENT_APP_SHUTDOWN:
                light_info("Received LF_EVENT_APP_SHUTDOWN", NULL);
                break;
        default:
                break;
        }
        
}
static uint8_t crush_app_main(struct light_application *app)
{
        light_debug("enter main task","");

        // an interactive console session needs more than one tick of its own periodic task to
        // finish -- see crush_console_is_active()'s comment in crush_private.h
        if(crush_console_is_active())
                return LF_STATUS_RUN;

        // default application lifecycle for command executables is run once then shutdown
        return LF_STATUS_SHUTDOWN;
}
static struct light_cli_invocation_result do_cmd_crush(struct light_cli_invocation *invoke){
        print_usage();
        return Result_Success;
}
struct light_command *crush_command_root(void)
{
        return &cmd_crush;
}
//   the command tree, printed from the tree itself rather than from a hardcoded line. What was
// here described `crush export`, a command that does not exist and by the look of it never did,
// and named none of the ten that do -- which is the failure mode of usage text maintained by
// hand. light_cli_print_command_help() walks the registered children instead, so this cannot
// drift from what the binary actually accepts
static void print_usage(void)
{
        light_cli_print_command_help(&cmd_crush);
}
