// CTPB.cpp : Cannonbeard's Treasure Picker with ship Bounties math
//
// 8/30/17 - copied DONDmp5pt to start this 
//				updated to handle different size pay tables
//				removed "opening cases" code
// 9/11/17 - copied CTP to add multiplier support
//				updated pay tables to new 72% tables
//				added ship hit/sunk counters
// 9/12/17 - added persistent ship value counters
//				added math method to calculate multiplier
// 9/13/17 - copied CTPM to change from multiplier to bounty
// 9/14/17 - fixed histo portion, added bins
// 9/15/17 - changed betraise to additional bets
//

#include "stdafx.h"
#include <time.h>
#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>

using namespace std;

// unweighted pay table 
class PayTable {
protected:
	vector<int> pay;							// value of the prizes
	int size;								// number of prizes

public:
	PayTable() {								// default contructor
		size = 0;

	}
	PayTable(const PayTable &ptObj) {			// copy constructor
		size = ptObj.size;
		//available = ptObj.available;
		for (int i = 0; i < size; i++) {
			pay.push_back(ptObj.pay.at(i));
			//closed[i] = ptObj.closed[i];
			//shuffled[i] = ptObj.shuffled[i];
		}
	}

	void setTable(const int pays[], int sz) {
		//cout << "PayTable.setTable: " << pays[0] << pays[1] << count << endl;
		size = sz;
		for (int i = 0; i < size; i++)
			pay.push_back(pays[i]);							// put pay into list
	}

	int getPay(int index) {										// return pay at index
		int retVal = -1;										// -1 is error code	
		if (index < size)										// if index is valid
			retVal = pay.at(index);								// return the pay
		return retVal;
	}

	int getTableSize() {
		return size;
	}

	string getPays(int bm) {
		string payString;
		for (int i = 0; i < size; i++) {
			payString = payString + to_string(pay[i] * bm) + " ";
		}
		return payString;
	}
}; // end PayTable class

// weighted pay table inherits from unweighted paytable, adds weights
class WPayTable : public PayTable {
	vector<int> weights;
	vector<int> totWeights;
	int totalWeight;

public:
	WPayTable() {								// default contructor
		size = 0;
		totalWeight = 0;
	}

	WPayTable(const WPayTable &ptObj) {			// copy constructor
		size = ptObj.size;
		totalWeight = ptObj.totalWeight;
		for (int i = 0; i < size; i++) {
			pay.push_back(ptObj.pay.at(i));
			weights.push_back(ptObj.weights.at(i));
			totWeights.push_back(ptObj.totWeights.at(i));
		}
	}

	void setTable(const int pays[], const int wts[], int sz) {
		//cout << "PayTable.setTable: " << pays[0] << pays[1] << count << endl;
		size = sz;
		for (int i = 0; i < size; i++) {
			pay.push_back(pays[i]);							// put pay into list
			weights.push_back(wts[i]);
		}
		totWeights.push_back(weights[0] - 1);
		for (int i = 1; i < size; i++) {
			totWeights.push_back(totWeights.at(i - 1) + weights.at(i));
		}
		totalWeight = totWeights.at(size - 1) + 1;
	}

	int getPay(int index) {										// find pay at index
		int retVal = -1;										// -1 is error code	
		if (index < totalWeight) {								// if index is valid 
			int j = 0;											// start at beginning of weighted list
			while (index > totWeights[j] && j<size) {			// if index is larger than this weight and we are not at end of array
				j++;											// increment to next total weight
			}
			retVal = pay.at(j);								// return the pay
		}
		return retVal;
	}

	int getTotalWeight() {
		return totalWeight;
	}
};

   /****************************************************************************************
   **	Set of paytables for a given number of players
   **
   **		pt = array of pay tables
   **		wt = arary of weights
   **		whichTable = which paytable is currently being used (0=pt1, 1=pt2, etc)
   **
   **		initialize object with constructor and/or set method
   **		playGame to randomly choose one of the paytables
   **
   **
   **
   ****************************************************************************************/
class PayTableSet {
	PayTable pt[5];
	int wt[5];
	int totWt[5];
	int whichTable;							// 0=pt1, 1=pt2, etc

public:
	PayTableSet() {
		// do nothing for default constructor
	}

	PayTableSet(const int p1[], int sz1,
		const int p2[], int sz2,
		const int p3[], int sz3,
		const int p4[], int sz4,
		const int p5[], int sz5, const int wts[]) {
		for (int i = 0; i < 5; i++) {
			wt[i] = wts[i];
		}
		totWt[0] = wt[0] - 1;
		for (int i = 1; i < 5; i++) {
			totWt[i] = totWt[i - 1] + wts[i];
		}

		pt[0].setTable(p1, sz1);

		pt[1].setTable(p2, sz2);

		pt[2].setTable(p3, sz3);

		pt[3].setTable(p4, sz4);

		pt[4].setTable(p5, sz5);

		whichTable = 0;
	}
	void set(const int p1[], int sz1,
		const int p2[], int sz2,
		const int p3[], int sz3,
		const int p4[], int sz4,
		const int p5[], int sz5, const int wts[]) {
		//cout << "in PayTableSet.set() " << p1[0] << " weight " << wts[0] << endl;
		for (int i = 0; i < 5; i++) {
			wt[i] = wts[i];
		}
		totWt[0] = wt[0] - 1;
		for (int i = 1; i < 5; i++) {
			totWt[i] = totWt[i - 1] + wts[i];
		}

		pt[0].setTable(p1, sz1);
		pt[1].setTable(p2, sz2);
		pt[2].setTable(p3, sz3);
		pt[3].setTable(p4, sz4);
		pt[4].setTable(p5, sz5);
		whichTable = 0;
	}
	/******************************************************************************
	** Use this to initiate a new game, it uses rng to decide which paytable to
	** use for this game
	*/
	void playGame() {
		int rndpt = 0;
		random_device ptRd;
		mt19937 ptGen(ptRd());
		uniform_int_distribution<> ptDist(0, (wt[0] + wt[1] + wt[2] + wt[3] + wt[4] - 1));
		rndpt = ptDist(ptGen);

		// determine which set will be used in next game
		//cout << "about to calculate next which Table, rndpt is " << rndpt << endl;
		//cout << "total weights are " << totWt[0] << " " << totWt[1] << " " << totWt[2] << " " << totWt[3] << " " << totWt[4] << " " << endl;
		int j = 0;													// start at beginning of weighted list
		while (rndpt > totWt[j] && j<4) {						// if index is larger than this weight and we are not at end of array
			j++;												// increment to next total weight
		}
		whichTable = j;
		//cout << "whichTable is " << whichTable << endl;
	}

	int getPay(int index) {
		int retWin = 0;
		retWin = pt[whichTable].getPay(index);

		return retWin;
	}

	string getPayValues(int bm) {				// bet multiplier
		string cv;
		cv = pt[whichTable].getPays(bm);
		return cv;
	}
}; // end PayTableSet class

class GameMath {
	string mathID;
	int mathVersion;
	int mathBasis;
	int paymodel;
	PayTableSet ps2p;
	PayTableSet ps3p;
	PayTableSet ps4p;
	WPayTable sb;							// ship bounties

public:
	GameMath() {							// default constructor
		mathID.append("CTPB");
		mathVersion = 1;
		mathBasis = 50;

		paymodel = 3;
		int weights2p[] = { 601, 1500, 300, 110, 151 };
		int weights3p[] = { 494, 1428, 403, 131, 164 };
		int weights4p[] = { 631, 1604, 350, 106, 170 };

		int p1[] = { 150, 70, 40, 30, 20, 10 };
		int p2[] = { 200, 100, 50, 30, 20, 10 };
		int p3[] = { 300, 120, 60, 40, 20, 10 };
		int p4[] = { 400, 150, 80, 40, 20, 10 };
		int p5[] = { 500, 200, 100, 50, 20, 10 };

		int p6[] = { 240, 120, 60, 48, 36, 24, 12 };
		int p7[] = { 300, 150, 90, 60, 36, 24, 12 };
		int p8[] = { 450, 210, 120, 84, 48, 24, 12 };
		int p9[] = { 600, 300, 150, 96, 48, 24, 12 };
		int p10[] = { 750, 360, 240, 120, 84, 48, 24 };

		int p11[] = { 300, 180, 120, 96, 60, 48, 24, 12 };
		int p12[] = { 420, 228, 156, 120, 72, 48, 24, 12 };
		int p13[] = { 600, 300, 180, 144, 108, 60, 24, 12 };
		int p14[] = { 840, 480, 240, 180, 120, 72, 36, 12 };
		int p15[] = { 1080, 540, 300, 216, 144, 96, 48, 24 };

		int bp[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25 };
		int bw[] = { 5, 10, 12, 12, 13, 25, 10, 3, 3, 16, 3, 3, 5 };

		ps2p.set(p1, sizeof(p1) / sizeof(int), p2, sizeof(p2) / sizeof(int),
			p3, sizeof(p3) / sizeof(int), p4, sizeof(p4) / sizeof(int),
			p5, sizeof(p5) / sizeof(int), weights2p);

		ps3p.set(p6, sizeof(p6) / sizeof(int), p7, sizeof(p7) / sizeof(int),
			p8, sizeof(p8) / sizeof(int), p9, sizeof(p9) / sizeof(int),
			p10, sizeof(p10) / sizeof(int), weights3p);

		ps4p.set(p11, sizeof(p11) / sizeof(int), p12, sizeof(p12) / sizeof(int),
			p13, sizeof(p13) / sizeof(int), p14, sizeof(p14) / sizeof(int),
			p15, sizeof(p15) / sizeof(int), weights4p);

		sb.setTable(bp, bw, sizeof(bp) / sizeof(int));
	}

	string getID() {						// getters
		return mathID;
	}

	int getVersion() {
		return mathVersion;
	}

	int getBasis() {
		return mathBasis;
	}

	int getPaymodel() {
		return paymodel;
	}

	void playGame(int players) {
		switch (players) {
		case 2:
			ps2p.playGame();
			break;
		case 3:
			ps3p.playGame();
			break;
		case 4:
			ps4p.playGame();
			break;
		default:
			cout << "Danger, Will Robinson! Number of players invalid in GameMath::playGame " << players << endl << endl;
			break;
		}
	}

	int getPay(int np, int index) {
		int retWin = 0;
		switch (np)
		{
		case 2:
			retWin = ps2p.getPay(index);
			break;
		case 3:
			retWin = ps3p.getPay(index);
			break;
		case 4:
			retWin = ps4p.getPay(index);
			break;
		default:
			cout << "Danger, Will Robinson! Number of players invalid in GameMath::getPay " << np << index << endl << endl;
			break;
		}
		return retWin;
	}

	string getPayValues(int np, int bm) {
		string cv;
		switch (np)
		{
		case 2:
			cv = ps2p.getPayValues(bm);
			break;
		case 3:
			cv = ps3p.getPayValues(bm);
			break;
		case 4:
			cv = ps4p.getPayValues(bm);
			break;
		default:
			cout << "Danger, Will Robinson! Number of players invalid in GameMath::getPayValues " << np << endl << endl;
			break;
		}
		return cv;
	}

	int getBountyTW() {
		return sb.getTotalWeight();
	}

	int getBounty(int index) {
		return sb.getPay(index);
	}
}; // end GameMath class

//
// Bins class implements a list of integer values and the frequency of each
//	Bins.record() takes an integer and adds it to the list of items
//	Bins.write() takes a string filename and appends all the bins to an output file
//
class Bins {
protected:
	vector<int> item;										// values
	vector<long> count;										// quantities
	int numBins;											// number of unique nonzero values
public:
	Bins() {
		numBins = 0;
		item.push_back(0);									// first bin is for zeroes
		count.push_back(0);									// initialize to 0
	}
	void record(int intVal) {
		boolean found = FALSE;
		if (intVal == 0) {
			count.at(0)++;
		}
		else {
			if (numBins == 0) {
				// we got the first nonzero win
				numBins++;
				item.push_back(intVal);
				count.push_back(1);
				found = TRUE;
			}
			else {
				// it's not the first win, see if it matches an existing win
				found = FALSE;
				int i = 1;
				while (i <= numBins) {
					if (item.at(i) == intVal) {					// we found a match
						found = TRUE;
						count.at(i)++;						// increment the counter for this win
						break;								// leave while loop
					}
					i++;									// move to next bin
				}// end while
				if (!found) {
					numBins++;								// we have a new win value
					item.push_back(intVal);						// add the win to the list
					count.push_back(1);						// initialize count to 1
				}
			}
		}
		return;
	}
	void write(string fn) {
		ofstream fh;
		fh.open(fn, ios::app);								// append to existing file
		if (fh.is_open()) {
			fh << "Value Count\n";
			for (int i = 0; i <= numBins; i++) {
				fh << item.at(i) << " " << count.at(i) << "\n";
			}
		}
		fh.close();
		return;
	}
}; // end of class Bins

int main()
{
	/**********************************************
	*  Constant Parameters
	**********************************************/
	const int BuyIn = 10000;						// default buy in is 10000 credits, or $100.00
	const int BetMult = 5;							// bet multiplier (number of 50 credit bets)
	const int RaiseMult = 5;						// bet raise additional multiplier (add to betMultiplier)

	/*********************************************
	*  Local variables defined
	*********************************************/
	char userInput = NULL;
	int numPlayers = 0;
	//double denom = 0.01;							// penny denomination
	int betsize = 0;								// size of a bet, determined by math
	int betMultiplier = BetMult;					// bet multiplier
	int betRaise = 0;
	int playerBalance[4] = { 0,0,0,0 };
	int shipHits[4] = { 0,0,0,0 };					// hits ship has taken, 5=sunk
	int persistShipValue[4] = { 0,0,0,0 };			// accumulate bets made, sum bet multipliers
	long long coinIn = 0;
	long long coinOut = 0;
	double actualHold = 0;
	GameMath gm1;
	string paysString;
	int index = 0;
	int winner;										// winning player 1 - 4
	int win;
	int shipBounty = 0;
	int totalWin = 0;								// win + ship bounties
	int sbtw = 0;									// ship bounty totalWeight of pay table
	int sbIndex = 0;								// index into weighted table
	random_device rd;
	mt19937 gen(rd());
	SYSTEMTIME startTime;							// so we can display start and end time, and create output filename
	SYSTEMTIME endTime;
	long long msStartTime;
	long long msEndTime;

	GetLocalTime(&startTime);
	msStartTime = startTime.wMilliseconds + 1000 * startTime.wSecond + 60 * 1000 * startTime.wMinute + 60 * 60 * 1000 * startTime.wHour;

	betsize = gm1.getBasis();

	// announce the game and get the game parameters (number of players)
	cout << endl << "Cannonbeard's Treasure Picker with Multipliers math" << endl;
	cout << "Starting at " << startTime.wMonth << "." << startTime.wDay << "  " << startTime.wHour << ":" << startTime.wMinute << ":" << startTime.wSecond << endl;
	cout << "Math: " << gm1.getID() << "  Version: " << gm1.getVersion() << "   Paymodel: " << gm1.getPaymodel() << endl;
	while (userInput != 'h' && userInput != 'i') {
		cout << "Enter h for histo or i for interactive: ";
		cin >> userInput;
	}

	if (userInput == 'i') {
		cout << endl << "Cannonbeard's Treasure Picker math interactive" << endl;

		while (numPlayers < 2 || numPlayers > 4) {
			cout << "Enter number of players (2-4): ";
			cin >> numPlayers;
		}
		uniform_int_distribution<> dist1(0, numPlayers - 1);		// which player wins
		uniform_int_distribution<> whichPay(0, numPlayers + 3);		// which pick
		sbtw = gm1.getBountyTW();
		uniform_int_distribution<> shipBountyDist(0, sbtw - 1);		// index into weighted pay table

		// create the log file and write game parameters
		string fn = "CTPM";
		ostringstream convert;
		GetLocalTime(&startTime);
		convert << "-" << startTime.wMonth << "-" << startTime.wDay << "-" << startTime.wYear << "-" << startTime.wHour << "-" << startTime.wMinute << "-" << startTime.wSecond << ".txt";
		fn.append(convert.str());
		ofstream outfile;
		outfile.open(fn);
		outfile << "CTPM_Interactive_GameMath" << endl << "Year_Month_Day " << startTime.wYear << " " << startTime.wMonth << " " << startTime.wDay << endl;
		outfile << "Hour_Minute_Second " << startTime.wHour << " " << startTime.wMinute << " " << startTime.wSecond << endl;
		outfile << "MathID " << gm1.getID() << endl;
		outfile << "Math_version " << gm1.getVersion() << endl;
		outfile << "Paymodel " << gm1.getPaymodel() << endl;
		outfile << "Math_basis " << gm1.getBasis() << endl;
		outfile << "Number_of_players " << numPlayers << endl;
		outfile.close();

		// initialize the players balances
		for (int i = 0; i < numPlayers; i++) {
			playerBalance[i] = BuyIn;					// assume $20 buy in for each player
		}

		// play the game
		do {
			outfile.open(fn, ios::app);								// append to existing file
			outfile << "Player1 Player2 Player3 Player4 CoinIn CoinOut" << endl;
			for (int i = 0; i < 4; i++) {
				outfile << playerBalance[i] << " ";
			};
			outfile << coinIn << " " << coinOut << " " << endl;
			outfile.close();
			cout << endl << "Player1    Player2    Player3    Player4    CoinIn   CoinOut   ActualHold" << endl;
			for (int i = 0; i < 4; i++) {
				cout << setw(7) << playerBalance[i] << "    ";
			};
			cout << setw(6) << coinIn << "   " << setw(7) << coinOut << "   ";
			cout << setw(10) << actualHold << "   " << endl;
			cout << setw(7) << shipHits[0] << "    " << setw(7) << shipHits[1] << "    " << setw(7) << shipHits[2] << "    " << setw(7) << shipHits[3] << " Hits on ships" << endl;
			cout << setw(7) << persistShipValue[0] << "    " << setw(7) << persistShipValue[1] << "    " << setw(7) << persistShipValue[2] << "    " << setw(7) << persistShipValue[3] << " Persistent ship values" << endl;
			cout << "Play, bet Raise, Add or Quit (p, r, a or q): ";
			cin >> userInput;
			betRaise = 0;
			switch (userInput) {
			case 'r':
				betRaise = RaiseMult;
			case 'p':
				cout << "Play game: " << betsize*(betMultiplier+betRaise) << " credits " << endl;
				// start the game, determine which paytables to use, show the players
				gm1.playGame(numPlayers);
				paysString = gm1.getPayValues(numPlayers, betMultiplier+betRaise);
				//cout << " numPlayers and betMultiplier are: " << numPlayers << "   " << betMultiplier << endl;
				cout << "Possible pays are: " << paysString << endl;
				outfile.open(fn, ios::app);								// append to existing file
				outfile << "Possible_pays_are: " << paysString << endl;
				outfile.close();
				// take bet from each player
				for (int i = 0; i < numPlayers; i++) {
					playerBalance[i] -= betsize*(betMultiplier+betRaise);
				}
				coinIn += betsize*(betMultiplier+betRaise)*numPlayers;
				// accumulate persistent data on ship values
				for (int i = 0; i < numPlayers; i++)
				{
					persistShipValue[i] += betMultiplier+betRaise;
				}

				// get size of pay table
				// get a random number
				// get pay at that index
				index = whichPay(gen);
				win = gm1.getPay(numPlayers, index)*(betMultiplier+betRaise);
				coinOut += win;

				// select winner
				winner = dist1(gen) + 1;

				cout << "Winning_player " << winner << endl;

				// figure out if any ships were sunk
				// get any multipliers
				// re-calculate win

				for (int i = 0; i < numPlayers; i++)
				{
					//cout << " winner and i are: " << winner << "  " << i << endl;
					if (i != (winner - 1)) {						// i is 0 to n-1, winner is 1 to n
						shipHits[i]++;
					}
					if (shipHits[i] == 5) {
						// get bounty, show message, reset hits, reset persist accumulator, add bounty to winner's balance
						sbIndex = shipBountyDist(gen);
						shipBounty = gm1.getBounty(sbIndex) * persistShipValue[i];
						cout << "ship " << i + 1 << " is sunk!  Value was " << persistShipValue[i] << " and bounty is " << shipBounty << endl;
						shipHits[i] = 0;
						persistShipValue[i] = 0;
						playerBalance[winner - 1] += shipBounty;
						coinOut += shipBounty;
					}
				}

				// calculate balances and holds
				playerBalance[winner - 1] += win;
				actualHold = ((double)coinIn - (double)coinOut) / (double)coinIn;

				cout << "Winner's payout is " << win << endl;

				outfile.open(fn, ios::app);								// append to existing file
				outfile << " winner_is_player " << winner << " win_is " << win << endl;
				outfile.close();
				break;
			case 'a':
				cout << "Add cash to players with low balances" << endl;
				for (int i = 0; i < numPlayers; i++)						// loop through players
					if (playerBalance[i] < betsize*betMultiplier)			// if balance is less than $2
						playerBalance[i] += BuyIn;							// then add $100 more
				break;
			case 'q':
				cout << endl << "Thank you for playing, good-bye." << endl << endl;
				GetLocalTime(&endTime);
				msEndTime = endTime.wMilliseconds + 1000 * endTime.wSecond + 60 * 1000 * endTime.wMinute + 60 * 60 * 1000 * endTime.wHour;
				outfile.open(fn, ios::app);								// append to existing file
				outfile << "start_time_milliseconds " << msStartTime << endl;
				outfile << "end_time_milliseconds " << msEndTime << endl;
				outfile << "session_duration_milliseconds " << msEndTime - msStartTime << endl;
				outfile.close();
				break;
			default:
				cout << endl << "I don't understand that input. Please try again." << endl;
				break;
			}
		} while (userInput != 'q');
	}
	else {								// histo mode, no interactive messages, log stats to file
		int iterations = 0;				// number of games to simulate
		int betRaiseProb = 0;			// chance of players doing a bet raise
		int rndRaise = 0;
		
		Bins whoWon;					// count winners as a check
		Bins handWins;					// count the hand wins
		Bins raiseWins;
		Bins bounties;					// count the ship bounties
		Bins totalWins;					// count total payouts
		Bins totalRaiseWins;
		Bins shipCntrs;					// count values of ship bet accumulators at bounty

		cout << endl << "Cannonbeard's Treasure Picker with Ship Bounties math bin histo" << endl;

		while (numPlayers < 2 || numPlayers > 4) {
			cout << "Enter number of players (2-4): ";
			cin >> numPlayers;
		}
		cout << "Enter probability of bet raise (0-100): ";
		cin >> betRaiseProb;
		cout << "Enter number of iterations: ";
		cin >> iterations;

		uniform_int_distribution<> dist1(0, numPlayers - 1);		// which player wins
		uniform_int_distribution<> whichPay(0, numPlayers + 3);		// which pick
		sbtw = gm1.getBountyTW();
		uniform_int_distribution<> shipBountyDist(0, sbtw - 1);		// index into weighted pay table
		uniform_int_distribution<> raiseProb(0, 99);				// probability of bet raise
		
		// create the log file and write game parameters
		string fn = gm1.getID();
		ostringstream convert;
		GetLocalTime(&startTime);
		convert << "-" << startTime.wMonth << "-" << startTime.wDay << "-" << startTime.wYear << "-" << startTime.wHour << "-" << startTime.wMinute << "-" << startTime.wSecond << ".txt";
		fn.append(convert.str());
		ofstream outfile;
		outfile.open(fn);
		outfile << "CTPB_Bin_Histo" << endl << "Year_Month_Day " << startTime.wYear << " " << startTime.wMonth << " " << startTime.wDay << endl;
		outfile << "Hour_Minute_Second " << startTime.wHour << " " << startTime.wMinute << " " << startTime.wSecond << endl;
		outfile << "MathID " << gm1.getID() << endl;
		outfile << "Math_version " << gm1.getVersion() << endl;
		outfile << "Paymodel " << gm1.getPaymodel() << endl;
		outfile << "Math_basis " << gm1.getBasis() << endl;
		outfile << "Bet_multiplier " << BetMult << endl;
		outfile << "Bet_raise " << RaiseMult << endl;
		outfile << "Bet_raise_probability " << betRaiseProb << endl;
		outfile << "Number_of_players " << numPlayers << endl;
		outfile << "Number_of_iterations " << iterations << endl;
		outfile.close();

		coinIn = 0;
		coinOut = 0;

		for (int i = 0; i < iterations; i++) {
			
			betRaise = 0;							// reset bet raise to no
			
			if (betRaiseProb > 0) {					// if bet raise, get rnd and check
				rndRaise = raiseProb(gen);
				if (rndRaise < betRaiseProb) {
					betRaise = RaiseMult;
				}
			}

			totalWin = 0;							// reset total win each game
			
				gm1.playGame(numPlayers);
			
				coinIn += betsize*(betMultiplier+betRaise)*numPlayers;
				// accumulate persistent data on ship values
				for (int i = 0; i < numPlayers; i++)
				{
					persistShipValue[i] += betMultiplier+betRaise;
				}

				// get size of pay table
				// get a random number
				// get pay at that index
				index = whichPay(gen);
				win = gm1.getPay(numPlayers, index)*(betMultiplier+betRaise);
				coinOut += win;
				totalWin = win;
				// select winner
				winner = dist1(gen) + 1;
				whoWon.record(winner);
				if (betRaise)
					raiseWins.record(win);
				else
					handWins.record(win);

				// figure out if any ships were sunk
				// get any multipliers
				// re-calculate win
				for (int i = 0; i < numPlayers; i++)
				{
					if (i != (winner - 1)) {						// i is 0 to n-1, winner is 1 to n
						shipHits[i]++;
					}
					if (shipHits[i] == 5) {
						// get bounty, show message, reset hits, reset persist accumulator, add bounty to winner's balance
						sbIndex = shipBountyDist(gen);
						shipBounty = gm1.getBounty(sbIndex) * persistShipValue[i];
						shipCntrs.record(persistShipValue[i]);
						shipHits[i] = 0;
						persistShipValue[i] = 0;
					
						coinOut += shipBounty;
						totalWin += shipBounty;
						bounties.record(shipBounty);
					}
				}
				if (betRaise)
					totalRaiseWins.record(totalWin);
				else
					totalWins.record(totalWin);
		}
		GetLocalTime(&endTime);
		msEndTime = endTime.wMilliseconds + 1000 * endTime.wSecond + 60 * 1000 * endTime.wMinute + 60 * 60 * 1000 * endTime.wHour;
		outfile.open(fn, ios::app);								// append to existing file
		outfile << "start_time_milliseconds " << msStartTime << endl;
		outfile << "end_time_milliseconds " << msEndTime << endl;
		outfile << "session_duration_milliseconds " << msEndTime - msStartTime << endl;
		outfile << "CoinIn " << coinIn << endl;
		outfile << "CoinOut " << coinOut << endl;
		outfile << "Winning_Player" << endl;
		outfile.close();
		whoWon.write(fn);
		outfile.open(fn, ios::app);								// append to existing file
		outfile << "Hand_Wins" << endl;
		outfile.close();
		handWins.write(fn);
		outfile.open(fn, ios::app);								// append to existing file
		outfile << "Raise_Wins" << endl;
		outfile.close();
		raiseWins.write(fn);
		outfile.open(fn, ios::app);								// append to existing file
		outfile << "Total_Wins" << endl;
		outfile.close();
		totalWins.write(fn);
		outfile.open(fn, ios::app);								// append to existing file
		outfile << "Total_Raise_Wins" << endl;
		outfile.close();
		totalRaiseWins.write(fn);
		outfile.open(fn, ios::app);								// append to existing file
		outfile << "Bounties" << endl;
		outfile.close();
		bounties.write(fn);
		outfile.open(fn, ios::app);								// append to existing file
		outfile << "Ship_bet_counters" << endl;
		outfile.close();
		shipCntrs.write(fn);
	}
	return 0;
}