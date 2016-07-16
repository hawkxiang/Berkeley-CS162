#pragma once
#include "job_control.h"

void cmd_launch(struct tokens*, const char* line);
void job_wait();
void switch_fg(pid_t pgid);
void switch_bg(pid_t pgid);
