
/**
 * Classe che modella il cronometro
 */

class LapTimer {
	public:
		LapTimer();
		int getLapNumber();
		long getCurrentLapTime();
		void startTimer();
		void stopTimer();
		void newLap();
		void sector();
	private:
		int lapCounter;
		long startLapTimestamp;
		long finishLapTimestamp;
};