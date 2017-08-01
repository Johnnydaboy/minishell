#define LINELEN 200000
extern int margc;
extern char **margv;
extern bool runFourFunctions;
extern int counterForShift;
extern bool enterShift;
extern bool normalExit;
extern int exitStatus;
extern char prompt[1024];
extern int processLine (char *buffer, char *expandBuffer, int fd[], int doWait);
extern int globalIntSigInt;
