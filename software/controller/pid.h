#ifndef _PID_H_
#define _PID_H_

struct pid
{
    /* public */
    float set;
    float actual;
    float Kp, Ki, Kd;
    float omin, omax;
    float separ_imin;
    float separ_imax;
    float imin, imax;

    /* private */
    float err;
    float err_last;
    float err_next;
    float integral;
    float out_last;
};

typedef struct pid *pid_t;

pid_t pid_create(void);
void  pid_delete(pid_t pid);
void  pid_reset(pid_t pid);

void  pid_config(pid_t pid, float Kp, float Ki, float Kd);
void  pid_set_integral_separation(pid_t pid, float separ_imin, float separ_imax);
void  pid_set_integral_limit(pid_t pid, float imin, float imax);
void  pid_set_output_limit(pid_t pid, float umin, float umax);
float pid_position_ctrl(pid_t pid, float set, float actual);
float pid_incremental_ctrl(pid_t pid, float set, float actual);

#endif  /* _PID_H_ */
