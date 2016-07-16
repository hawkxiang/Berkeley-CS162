#pragma once
#include "init_shell.h"

void launch_job (job *j, int foreground);

void update_status();

void wait_background();

void continue_job(job* j, int foreground);
