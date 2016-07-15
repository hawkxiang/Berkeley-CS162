#pragma once
#include "init_shell.h"

void launch_process(process *p, pid_t pgid, 
        int infile, int outfile, int errfile, int foreground);

void launch_job (job *j, int foreground);
