#define SCANBACK_BUTTON		1
#define PLAY_BUTTON			2	
#define SCANFORWARD_BUTTON	3
#define MUTE_BUTTON			4

void debounceInit(void);
int buttonPressed(void);
int buttonReleased(void);
int buttonNPressed(int n);
int buttonDown(void);
int buttonNDown(int n);