/**
 *  Copyright (C) 2020 by Hubertus Hans (Hubertus.Hans@imatek.tech)
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
 *  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 *  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *  Language  : C/C++11
 */
 
 /*
 *  UTS Data Structure
 *  Nama		: Hubertus Hans
 *  NIM			: 2301915821
 *  Kelas		: LB-01
 *  IDE			: Microsoft Visual Studio Enterprise 2017
 *  Compiler	: Microsoft Visual C++ 
 *  Keterangan	: Hanya dapat berjalan di Windows karena menggunakan Windows API untuk memanipulasi tampilan Console
 */



#include "pch.h"      //header file untuk compile di Visual Studio 
#include <iostream>
#include <windows.h>
#include <string>
#include <thread>
#include<fstream>
using namespace std;

//menentukan ETC( Estimated Time to Complete) masing masing fitur dalam detik untuk mobil kecil dan sedang
const int CUCI = 20;
const int VAKUM = 10;
const int POLES = 45;
						// Cuci   Vakum  Poles
const int biaya[4][3] = { {50000, 35000, 125000},     //mobil kecil : ayla , jazz
						  {60000, 40000, 150000},     //sedang: mpv, avanza, rush, brw
						  {70000, 50000, 200000},     //besar: alphard, fortuner
						  {70000, 40000, 0} };        //TRUCK

const string logFile = "Log.txt";

string center(string msg, int len, char spacer = ' ') {
	int sl = len - msg.length();
	string hasil = "";
	if (sl % 2 != 0) hasil += spacer;
	for (int i = 0; i < sl / 2; i++)hasil += spacer;
	hasil += msg;
	for (int i = 0; i < sl / 2; i++)hasil += spacer;
	return hasil;
}
string left(string msg, int len, char spacer = ' ') {
	int sl = len - msg.length();
	string hasil = msg;
	for (int i = 0; i < sl; i++)hasil += spacer;
	return hasil;
}
string right(string msg, int len, char spacer = ' ') {
	int sl = len - msg.length();
	string hasil = "";
	for (int i = 0; i < sl; i++)hasil += spacer;
	hasil += msg;
	return hasil;
}

struct mobil {
	char plat[9];
	char kendaraan[10];
	int type;
	bool layanan[3]; //[0]:cuci [1]:vakum, [2]:poles
	int biaya;
	int jamMasuk[2];
	int ETC;
	int duration;
	mobil* next;
};

class Console {
public:
	void print(int x, int y,string msg)
	{
		goTo(x, y);
		cout << msg;
	}
	void println(int x, int y, string msg) {
		print(x, y, msg + "\n");
	}
	void goTo(int x, int y) {
		COORD pos = { x, y };
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
	}
	int whereX()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		return csbi.dwCursorPosition.X;
	}
	int whereY()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		return csbi.dwCursorPosition.Y;
	}
	void clearConsole() {
		goTo(0, 9);
		string whiteSpace = " ";
		for (int i = 0; i < 110; i++)whiteSpace += " ";
		for (int i = 0; i < 22; i++)println(0, i + 9, whiteSpace);
		goTo(0, 10);
	}
};



Console console;
QueueMobil antrian[4]; //antrian 0 khusus truck dan besar, 1-3 kecil dan sedang
QueueMobil selesai;

int i = 0, last = 10,mode=0;
bool ticker_EN = true;

void writeLog(mobil data) {
	ofstream log;
	int c=0, v=0, p=0,jm[3],jk[2];
	jm[0] = data.jamMasuk[0];
	jm[1] = data.jamMasuk[1];
	jm[2] = 0;
	jk[0] = data.duration / 60;
	jk[1] = data.duration % 60;
	string jamM = right(to_string(jm[0]), 2, '0') + ":" + right(to_string(jm[1]), 2, '0') + ":" + right(to_string(jm[2]), 2, '0');
	string jamk = right(to_string(jm[0]), 2, '0') + ":" + right(to_string(jm[1] + jk[0]), 2, '0') + ":" + right(to_string(jm[2] + jk[1]), 2, '0');
	log.open(logFile, ofstream::out | ofstream::app);
	log << data.type<<','<<data.kendaraan<<','<<data.plat<<',';
	for (int i = 0; i < 3; i++)log << data.layanan[i] << ',';
	log << jamM << ',' << jamk << ',' << data.biaya << endl;
	log.close();
}

void refreshBay(int no) {
	if (mode != 1)return;
	int lastx = console.whereX();
	int lasty = console.whereY();
	int x = (no - 1) * 25;
	string jenis = " ", plat = "EMPTY", masuk = " ", ETC = " ", que = "in Queue: 0",service=" ";
	if (antrian[no-1].qcount() > 0) {
		mobil now = antrian[no-1].front();
		if (now.ETC == 0) {
			selesai.enqueue(now.plat, now.kendaraan, now.type, now.layanan[0], now.layanan[1], now.layanan[2], now.biaya, now.jamMasuk);
			antrian[no - 1].dequeue();
			return refreshBay(no);
		}
		else {
			jenis = now.kendaraan;
			plat = now.plat;
			masuk = to_string(now.jamMasuk[0]) + ":" + to_string(now.jamMasuk[1]);
			ETC = "ETC: " + to_string(now.ETC);
			service = "";
			if (now.layanan[0])service += "C ";
			if (now.layanan[1])service += "V ";
			if (now.layanan[2])service += "P ";
			que = "in queue: " + to_string(antrian[no - 1].qcount() - 1);
		}
	}
	string nama = center("BAY " + to_string(no),19);
	console.println(x, 1, "---------------------");
	console.println(x, 2, "|"+nama+"|");
	console.println(x, 3, "|"+center(jenis,19)+"|");
	console.println(x, 4, "|"+center(plat,19)+"|");
	console.println(x, 5, "|"+ center(service, 19) +"|");
	console.println(x, 6, "|" + center(ETC, 19) + "|");
	console.println(x, 7, "|"+center(que,19)+"|");
	console.println(x, 8, "---------------------");
	console.goTo(lastx,lasty);
}

void printDaftarHarga() {
	cout << "------------------------------------------------------------------------------------------------------------" << endl;
	cout << "|" << center(" ", 12) << "|" << center(" ", 32) << "|" << center("Harga", 60) << "|" << endl;
	cout << "|" << center("Kode", 12) << "|" << center("Jenis Kendaraan", 32) << "|" << center("-", 60, '-') << "|" << endl;
	cout << "|" << center(" ", 12) << "|" << center(" ", 32) << "|" << center("Cuci", 19) << "|" << center("Vakum", 19) << "|" << center("Poles", 20) << "|" << endl;
	cout << "------------------------------------------------------------------------------------------------------------" << endl;
	for (int i = 0; i < 4; i++) {
		string jenis;
		switch (i)
		{
		case 0:
			jenis = "Kecil: Ayla, Jazz";
			break;
		case 1:
			jenis = "Sedang: MPV, Avanza, Rush, BRV";
			break;
		case 2:
			jenis = "Besar: Fortuner, Alphard";
			break;
		case 3:
			jenis = "Truck";
			break;
		}
		if (biaya[i][2] == 0) cout << "|" << center(to_string(i + 1), 12) << "|" << left(jenis, 32) << "|" << center(to_string(biaya[i][0]), 19) << "|" << center(to_string(biaya[i][1]), 19) << "|" << center("-", 20) << "|" << endl;
		else cout << "|" << center(to_string(i + 1), 12) << "|" << left(jenis, 32) << "|" << center(to_string(biaya[i][0]), 19) << "|" << center(to_string(biaya[i][1]), 19) << "|" << center(to_string(biaya[i][2]), 20) << "|" << endl;
	}
	cout << "------------------------------------------------------------------------------------------------------------" << endl;
}

void ticker() {
	while (ticker_EN) {
		for (int j = 1; j < 5; j++) {
			antrian[j - 1].tick();
			refreshBay(j);
		}
		Sleep(1000);
	}
}

void Reg() {
	system("CLS");
	cin.clear();
	console.goTo(0, 9);
	string plat="", jam="",kendaraan="",jenis="";
	char in=0;
	bool c = 0, v = 0, p = 0;
	printDaftarHarga();
	cout << endl;
	cout << center("Form Registrasi Kendaraan", 100) << endl << endl;
	int lasty = console.whereY();
	console.print(0, lasty,  "Jenis Kendaraan (1-4) : ");
	console.print(40, lasty, "No Plat (D1234XXX) : ");
	console.print(0, lasty+1,"Tipe Kendaraan [1..10]: ");
	console.print(40, lasty+1, "Jam Masuk (XX:XX)  : ");
	console.goTo(24, lasty);
	cin >> jenis; cin.clear();
	console.goTo(24, lasty + 1);
	cin >> kendaraan; cin.clear();
	console.goTo(63, lasty);
	cin >> plat; cin.clear();
	console.goTo(63, lasty+1);
	cin >> jam; cin.clear();
	console.println(0, lasty+2, "Services(y/n):");
	console.print(15, lasty+3,   "Cuci  : ");
	console.print(15, lasty + 4, "Vakum : ");
	if (stoi(jenis) <= 3) console.print(15, lasty + 5, "Poles : ");
	console.goTo(23, lasty + 3);
	cin >> in; cin.clear();
	if (in == 'y')c = 1;
	console.goTo(23, lasty + 4);
	cin >> in; cin.clear();
	if (in == 'y')v = 1;
	if (stoi(jenis) < 4) {
		console.goTo(23, lasty + 5);
		cin >> in; cin.clear();
		if (in == 'y')p = 1;
	}
	cout << "Confirm (y/n)? "; cin >> in;
	if (in == 'y') {
		int type = stoi(jenis);
		int cost = (int)c*biaya[type - 1][0] + (int)v*biaya[type - 1][1] + (int)p*biaya[type - 1][2];
		int jamm[2] = {stoi(jam.substr(0,2)),stoi(jam.substr(3,2)) };
		if (stoi(jenis) < 3) {
			int lowest=antrian[1].qcount(), index=1;
			if (antrian[2].qcount() < lowest) {
				lowest = antrian[2].qcount(); index = 2;
			}
			if (antrian[3].qcount() < lowest) {
				lowest = antrian[3].qcount(); index = 3;
			}
			antrian[index].enqueue((char*)plat.c_str(), (char*)kendaraan.c_str(), stoi(jenis), c, v, p, cost, jamm);
		}
		else {
			antrian[0].enqueue((char*)plat.c_str(), (char*)kendaraan.c_str(), stoi(jenis), c, v, p, cost, jamm);
		}
	}

}

void Bayar() {
	system("CLS");
	cin.clear();
	cout << "---------------------------------------------------------------------------------------------------------" << endl;
	cout << "|" << center("Daftar yang sudah dikerjakan", 103) << "|" << endl;
	selesai.printAll(1);
	cout << endl;
	if (selesai.qcount() > 0) {
		string plat;
		cout << "No Plat: "; cin >> plat;
		mobil data = selesai.wherePlat((char*)plat.c_str());
		int jm[3], jk[2];
		jm[0] = data.jamMasuk[0];
		jm[1] = data.jamMasuk[1];
		jm[2] = 0;
		jk[0] = data.duration / 60;
		jk[1] = data.duration % 60;
		cout <<endl<< "Detail transaksi:" << endl;
		cout << "    Jenis Mobil : " << data.kendaraan << endl;
		cout << "    Plat Mobil  : " << data.plat<< endl;
		cout << "    Jam Masuk   : " << right(to_string(jm[0]), 2, '0') + ":" + right(to_string(jm[1]), 2, '0') + ":" + right(to_string(jm[2]), 2, '0') << endl;
		cout << "    Jam Keluar  : " << right(to_string(jm[0]), 2, '0') + ":" + right(to_string(jm[1]+jk[0]), 2, '0') + ":" + right(to_string(jm[2]+jk[1]), 2, '0') << endl;
		cout << "    Layanan" << endl;
		if(data.layanan[0])cout << "        Cuci    : " << biaya[data.type-1][0] << endl;
		if (data.layanan[1])cout << "        Vakum   : " << biaya[data.type-1][1] << endl;
		if(data.layanan[2] && data.type<4)cout << "        Poles   : " << biaya[data.type-1][2] << endl;
		cout << endl<<"    Total       : " <<data.biaya<< endl;
		writeLog(data);
		selesai.popPlat((char*)plat.c_str());
	}
}

void viewQueue() {
	system("CLS");
	cin.clear();
	console.goTo(0, 9);
	for (int i = 0; i < 4; i++) {
		console.clearConsole();
		cout << "----------------------------------------------------------------------------------" << endl;
		cout << "|" << center("Bay " + to_string(i + 1), 80) << "|" << endl;
		antrian[i].printAll();
		system("PAUSE");
	}
}

void Transaksi() {
	system("CLS");
	cin.clear();
	cout << "---------------------------------------------------------------------------------------------------------" << endl;
	cout << "|" << center("Daftar Transaksi", 103) << "|" << endl;
	cout << "---------------------------------------------------------------------------------------------------------" << endl;
	cout << "|" << center("Kode", 12) << "|" << center("Mobil", 20) << "|" << center("Plat", 10) << "|" << center("Cuci", 7) << "|" << center("Vakum", 7) << "|" << center("Poles", 7) << "|" << center("Jam Masuk", 11) << "|" << center("Jam Keluar", 11) << "|" << center("Total", 10) << "|" << endl;
	cout << "---------------------------------------------------------------------------------------------------------" << endl;
	ifstream logs(logFile);
	for (string line; getline(logs, line); )
	{
		string data[9];
		int count = 0;
		for (int x = 0; x < line.length(); x++) {
			if (line[x] == ',')count++;
			else data[count] += line[x];
		}
		cout << "|" << center(data[0], 12) << "|" << center(data[1], 20) << "|" << center(data[2], 10) << "|" << center(data[3], 7) << "|" << center(data[4], 7) << "|" << center(data[5], 7) << "|" << center(data[6], 11) << "|" << center(data[7], 11) << "|" << center(data[8], 10) << "|" << endl;
	}
	cout << "---------------------------------------------------------------------------------------------------------" << endl;
}

int main()
{
	console.println(0, 0, center("WASH - WASH", 98));
	console.clearConsole();
	thread timer(ticker);
	system("PAUSE");
	console.clearConsole();
	int p = 0;
	while (p != 5) {
		system("CLS");
		system("CLS");
		console.println(0, 0, center("WASH - WASH", 98));
		mode = 1;
		console.clearConsole();
		cout << "Menu:" << endl;
		cout << "1. Registrasi" << endl;
		cout << "2. Lihat Antrian" << endl;
		cout << "3. Bayar" << endl;
		cout << "4. Rekap Transaksi" << endl;
		cout << "5. Keluar" << endl;
		cout << "Masukan pilihan: ";
		cin >> p;
		console.clearConsole();
		switch (p) {
		case 1:
			mode = 1;
			Reg();
			system("PAUSE");
			break;
		case 2:
			mode = 1;
			viewQueue();
			break;
		case 3:
			mode = 3;
			Bayar();
			system("PAUSE");
			break;
		case 4:
			mode = 4;
			Transaksi();
			system("PAUSE");
			break;
		case 5: 
			ticker_EN = false;
			system("CLS");
			break;
		}
	}
	timer.join();
	
}

