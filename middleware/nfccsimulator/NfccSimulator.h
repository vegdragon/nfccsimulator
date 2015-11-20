#ifdef __NFCCSIMULATOR_H__
#define __NFCCSIMULATOR_H__

enum NfccStates
{
	STATE_OFF,
	STATE_ON,
	STATE_INITIALIZED,
	STATE_SHUTDOWN
};

class NfccSimulator
{
public:
	void init();
	void bootup();
	void shutdown();

private:
	NfccStates 	_state;
	int 		_fdOfNfcDriver;
};

#endif /* __NFCCSIMULATOR_H__ */