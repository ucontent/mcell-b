#ifndef CHKPT_H
#define CHKPT_H

/* header file for chkpt.c, MCell checkpointing functions */

#define CHKPT_BUFSIZE 10000
#define IO_COUNT 10000
#define MCELL_BIG_ENDIAN 16
#define MCELL_LITTLE_ENDIAN 17

#define CURRENT_TIME_CMD 1
#define CURRENT_ITERATION_CMD 2
#define CHKPT_SEQ_NUM_CMD 3
#define RNG_STATE_CMD 4
#define RELEASE_EVENT_CMD 5
#define MOLECULE_CMD 6
#define EFFECTOR_CMD 7
#define RX_STATE_CMD 8
#define BYTE_ORDER_CMD 9
#define MCELL_VERSION_CMD 10

int write_chkpt(FILE *fs);
int read_chkpt(FILE *fs);
int write_mcell_version(FILE *fs);
int read_mcell_version(FILE *fs);
int write_current_time(FILE *fs);
int read_current_time(FILE *fs);
int write_current_iteration(FILE *fs);
int read_current_iteration(FILE *fs);
int write_chkpt_seq_num(FILE *fs);
int read_chkpt_seq_num(FILE *fs);
int write_rng_state(FILE *fs);
int read_rng_state(FILE *fs);
int write_free_molecules(FILE *fs);
int read_free_molecule(FILE *fs);
int write_grid_molecules(FILE *fs);
int read_grid_molecule(FILE *fs);
int write_rx_states(FILE *fs);
int read_rx_state(FILE *fs);
int write_byte_order(FILE *fs);
int read_byte_order(FILE *fs);

#endif