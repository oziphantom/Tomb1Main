#include "args.h"

#include "util.h"

#include <windows.h>
#include <shellapi.h>
#include <string.h>

int get_command_line(char ***args, int *arg_count)
{
    LPWSTR *l_arg_list;
    int l_arg_count;

    l_arg_list = CommandLineToArgvW(GetCommandLineW(), &l_arg_count);
    if (!l_arg_list) {
        LOG_ERROR("CommandLineToArgvW failed");
        return 0;
    }

    *args = malloc(l_arg_count * sizeof(char **));
    *arg_count = l_arg_count;
    for (int i = 0; i < l_arg_count; i++) {
        size_t size = wcslen(l_arg_list[i]);          //this is the size without the NULL terminator
        (*args)[i] = malloc(size+1);                  //alloc one more to hold the null
        wcstombs((*args)[i], l_arg_list[i], size+1);  //allow it to also look at the NULL so the string is terminated
    }

    LocalFree(l_arg_list);

    return 1;
}
