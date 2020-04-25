/*
 *  UTS Data Structure
 *  Nama		: Hubertus Hans
 *  NIM			: 2301915821
 *  Kelas		: LB-01
 *  IDE			: Microsoft Visual Studio Enterprise 2017
 *  Compiler	: Microsoft Visual C++ 
 *  Keterangan	: Hanya dapat berjalan di Windows karena menggunakan Windows API untuk memanipulasi tampilan Console
 *  Asumsi		: 
 *					1. Tempat pencucian mobil memiliki 4 tempat pengerjaan dimana tempat pertama hanya dapat menampung mobil besar dan truck sedangkan 
 *						3 tempat lainnya hanya dapat menampung mobil kecil dan sedang
 *					2. Tiap tempat pengerjaan memiliki antriannya masing masing (Konsep seperti antrian gerbang tol)
 *					3. Jam masuk di-input manual dan durasi pengerjaan sudah di tentukan di program
 */



#include "pch.h"      //header file untuk compile di Visual Studio 
#include <iostream>   //header untuk IO konsol
#include <windows.h>  //header untuk WINAPI
#include <string>     //header untuk menggunakan tipe data string
#include <thread>	  //header untuk mendukung penggunaan thread
#include<fstream>     //header untuk mendukung baca tulis file
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

const string logFile = "Log.txt"; //nama file untuk menyimpan transaksi yang telah terjadi

//fungsi - fungsi padding string:
string center(string msg, int len, char spacer = ' ') {
	int sl = len - msg.length(); //menentukan space yang perlu diisi
	string hasil = "";
	if (sl % 2 != 0) hasil += spacer;
	for (int i = 0; i < sl / 2; i++)hasil += spacer;
	hasil += msg;
	for (int i = 0; i < sl / 2; i++)hasil += spacer;
	return hasil;
}  
string left(string msg, int len, char spacer = ' ') {  
	int sl = len - msg.length(); //menentukan space yang perlu diisi
	string hasil = msg;
	for (int i = 0; i < sl; i++)hasil += spacer;
	return hasil;
}
string right(string msg, int len, char spacer = ' ') {  
	int sl = len - msg.length(); //menentukan space yang perlu diisi
	string hasil = "";
	for (int i = 0; i < sl; i++)hasil += spacer;
	hasil += msg;
	return hasil;
}

string Rupiah(int val, char sep = '.'){
	//merubah integer menjadi string berformat 
	string value = to_string(val);
	int len = value.length();
	int dlen = 3;

	while (len > dlen) {
		value.insert(len - dlen, 1, sep);
		dlen += 4;
		len += 1;
	}
	return "Rp." + value;
}


struct mobil {  //membuat struktur data baru dengan tipe mobil
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

class Console { //Windows API Console manipulation implementation
	/* Class pengimplementasian windows API untuk memanipulasi tampilan konsol
	 * berisikan beberapa fungsi:
	 *	print -> mencetak string pada koordinat tertentu
	 *  println -> mencetak string pada koordinat terntentu dengan penambahan enter pada akhir
	 *  goTo -> memindahkan kursor ke posisi yang diinginkan
	 *  whereX -> mendapatkan posisi kursor terakhir kali pada sumbu x
	 *  whereY -> Mendapatkan posisi kursor terakhir kali pada sumbu y
	 *  clearConsole-> menghapus sebagian area kerja dengan cara menimpa dengan whitespace(spasi)
	 */
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

class QueueMobil { //implementasi linked list dalam bentuk queue
	/*
	 *  implementasi single linked list untuk membentuk queue,
	 *  terdiri dari kumpulan fungsi yang dibuat untuk memudahkan penggunaan link list pada main program
	 *  fungsi fungsi yang tersedia sebagai berikut:
	 *		enqueue		-> memasukan data ke queue yaitu insert belakang pada linked list
	 *		front		-> mendapatkan data terdepan dalam antrial, mengreturn objek mobil
	 *		qcount		-> mendapatkan jumlah antrian
	 *		wherePlat	-> mendapatkan data kendaraan dengan plat tertentu
	 *		popPlat		-> menghapus data dengan plat tertentu implementsi hapus depan,tengah,belakang
	 *		tick		-> fungsi untuk mengurangi estimated time pada mobil paling depan
	 *		dequeue		-> menghapus antrian paling depan yaitu hapus depan
	 *		printAll	-> mencetak seluruh data yang ada dalam queue
	 */
private: 
	mobil* head=0; 
	mobil* tail=0;
	mobil* newnode;
	int count=0;
public:  
	void enqueue(char plat[], char kendaraan[], int type, bool cuci, bool vakum, bool poles,int biaya, int jam[]) { //insert belakang
		newnode = new mobil;
		strcpy_s(newnode->plat, plat);
		strcpy_s(newnode->kendaraan, kendaraan);
		newnode->type = type;
		newnode->layanan[0] = cuci;
		newnode->layanan[1] = vakum;
		newnode->layanan[2] = poles;
		newnode->jamMasuk[0] = jam[0];
		newnode->jamMasuk[1] = jam[1];
		newnode->ETC = (int)cuci*CUCI + (int)vakum*VAKUM + (int)poles*POLES;
		if (type >=3) newnode->ETC *= 2;
		newnode->duration = newnode->ETC;
		newnode->biaya = biaya;
		newnode->next = NULL;
		if (head == 0) {
			head = newnode;
			tail = newnode;
		}
		else {
			tail->next = newnode;
			tail = newnode;
		}
		count++;
	}
	mobil front() {  //ambil data pling depan
		mobil* tmp = head;
		return *tmp;
	}
	int qcount() { //ambil jumlah antrian
		return count;
	}
	mobil wherePlat(char plat[]) { //ambil data spesifik
		mobil* tmp = head;
		while (tmp != NULL) {
			if (strcmp(tmp->plat, plat) == 0) {
				return *tmp;
			}
			tmp = tmp->next;
		}
	}
	void popPlat(char plat[]) {  //hapus data spesifik
		mobil* tmp = head;
		if (strcmp(head->plat, plat) == 0) {
			dequeue();
		}
		else {
			while (tmp->next != NULL && strcmp(tmp->next->plat, plat) != 0) {
				tmp = tmp->next;
			}
			mobil* ptr = tmp->next;
			if (ptr->next != NULL) tmp->next = ptr->next;
			else {
				tmp->next = NULL;
				tail = tmp;
			}
			free(ptr);
		}
	} 
	void tick() {  //fungsi untuk mengurangi etc
		if(head!=0)head->ETC -= 1;
	}
	void dequeue() {  //hapus depan
		if (count > 1) {
			mobil* tmp = head;
			head = head->next;
			free(tmp);
			count--;
		}
		else if (count == 1) {
			mobil* tmp = head;
			head = 0;
			tail = 0;
			free(tmp);
			count--;
		}
	} 
	void printAll(int mode=0) {  //mencetak semua data ke konsol
		if (mode == 0) {
			cout << "----------------------------------------------------------------------------------" << endl;
			cout << "|" << center("Kode", 12) << "|" << center("Mobil", 20) << "|" << center("Plat", 10) << "|" << center("Cuci", 7) << "|" << center("Vakum", 7) << "|" << center("Poles", 7) << "|" << center("Jam Masuk", 11) << "|" << endl;
			cout << "----------------------------------------------------------------------------------" << endl;
		}
		else {
			cout << "-----------------------------------------------------------------------------------------------------------" << endl;
			cout << "|" << center("Kode", 12) << "|" << center("Mobil", 20) << "|" << center("Plat", 10) << "|" << center("Cuci", 7) << "|" << center("Vakum", 7) << "|" << center("Poles", 7) << "|" << center("Jam Masuk", 11) << "|" << center("Jam Keluar", 11) << "|" << center("Total", 12) << "|" << endl;
			cout << "-----------------------------------------------------------------------------------------------------------" << endl;
		}
		mobil* ptr = head;
		while (ptr!= NULL) {
			int k, jm[3], jk[2];
			string mobil,p;
			string cc=" ", vv=" ", pp=" ";
			k = ptr->type;
			mobil = ptr->kendaraan;
			p = ptr->plat;
			jm[0] = ptr->jamMasuk[0];
			jm[1] = ptr->jamMasuk[1];
			jm[2] = 0;
			jk[0] = ptr->duration / 60;
			jk[1] = ptr->duration % 60;
			if (ptr->layanan[0])cc = "V";
			if (ptr->layanan[1])vv = "V";
			if (ptr->layanan[2])pp = "V";
			if (mode == 0) cout << "|" << center(to_string(k), 12) << "|" << center(mobil, 20) << "|" << center(p, 10) << "|" << center(cc, 7) << "|" << center(vv, 7) << "|" << center(pp, 7) << "|" << center(right(to_string(jm[0]) ,2,'0')+":"+right(to_string(jm[1]),2,'0') + ":"+ right(to_string(jm[2]),2,'0'), 11) << "|" << endl;
			else cout << "|" << center(to_string(k), 12) << "|" << center(mobil, 20) << "|" << center(p, 10) << "|" << center(cc, 7) << "|" << center(vv, 7) << "|" << center(pp, 7) << "|" << center(right(to_string(jm[0]), 2, '0') + ":" + right(to_string(jm[1]), 2, '0') + ":" + right(to_string(jm[2]), 2, '0'), 11) << "|" << center(right(to_string(jm[0]), 2, '0') + ":" + right(to_string(jm[1]+jk[0]), 2, '0') + ":" + right(to_string(jm[2]+jk[1]), 2, '0'), 11) << "|" <<center(Rupiah(ptr->biaya),10)<<"|"<< endl;
			ptr = ptr->next;
		}
		if (mode == 0) cout << "----------------------------------------------------------------------------------" << endl;
		else cout << "-----------------------------------------------------------------------------------------------------------" << endl;
	}
}; 

Console console;  //deklarasi objek Console dengan nama console
QueueMobil antrian[4]; //deklarasi objek Queue dengan nama antrian berbentuk array dengan index 0 khusus truck dan besar, 1-3 kecil dan sedang
QueueMobil selesai; //deklarasi objek Queue dengan nama selesai diperuntukan untuk menyimpan data mobil yang telah selesai namun belum dibayar

int i = 0,mode=0;
bool ticker_EN = true;  //flag untuk memberhentikan thread


void writeLog(mobil data) { //fungsi untuk menyimpan data pada file dengan nama file yang telah ditentukan diatas
	/*
	 *	membuka file lalu menuliskan data sesuai format yang dikehendaki lalu menutup file
	 */
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

void refreshBay(int no) {  //cetak info pengerjaan realtime di masing-masing antrian
	/*
	 *algo:
	 *  simpan posisi terakhir kursor lalu periksa apakah queue tertentu ada isinya
	 *  jika ada lalu ambil data terdepan dan periksa apakah ETC( estimated time to complete) =0,
	 *  jika ETC 0 maka masukan data ke antrian selesai dan memanggil diri sendiri kembali untuk mencetak data yang dikerjakan
	 *  jika tidak 0, maka bentuk data yang ada ke dalam string lalu tampilkan sesuai format yang diinginkan
	 *  setelah selesai, kembalikan kursor ke tempat semula
	 */
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
	/*
	 * mencetak daftar harga dari aray 2 dimensi tempat menyimpan harga tiap layanan
	 */
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
	/*
	 * thread baru untuk mengurangi counter estimasi secara berkala  dan mengupdate tampilan realtime pengerjaan tempat 1 - 4
	 */
	while (ticker_EN) {
		for (int j = 1; j < 5; j++) {
			antrian[j - 1].tick();
			refreshBay(j);
		}
		Sleep(1000);
	}
}

void Reg() {
	/*
	 *  mendaftarkan kendaraan baru / mobil masuk
	 *  algo:
	 *		meminta masukan semua data mobil yang akan dikerjakan termasuk layanan dan jam masuk
	 *      jika tipe kendaraan adalah mobil besar atau truck maka kendaraan langsung dimaksukan ke antrian tempat 1
	 *      sedangkan jika kendaraan adalah mobil kecil atau sedang, maka akan diperiksa antara tempat 2 - 4 yang antriannya paling sedikit
	 *		lalu kendaraan dimasukan ke antrian yang paling sedikit tersebut
	 */
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
	/*
	 *  algo:
	 *		mengambil data yang ada di dalam antrian bayar
	 *		jika ada data maka menampilkan masukan momor plat ang hendak dibayar
	 *		jika plat bukan 'c' maka akan ditampilkan rincian dan otomatis akan terhapus data mobil tersebut dari antrian bayar
	 *		lalu data akan dituliskan di dalam file txt sebagai catatan transaksi 
	 */
	system("CLS");
	cin.clear();
	cout << "-----------------------------------------------------------------------------------------------------------" << endl;
	cout << "|" << center("Daftar yang sudah dikerjakan", 105) << "|" << endl;
	selesai.printAll(1);
	cout << endl;
	if (selesai.qcount() > 0) {
		string plat;
		cout << "No Plat (c : cancel): "; cin >> plat;
		if (plat == "c")return;
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
		if(data.layanan[0])cout << "        Cuci    : " << Rupiah(biaya[data.type-1][0]) << endl;
		if (data.layanan[1])cout << "        Vakum   : " << Rupiah(biaya[data.type-1][1]) << endl;
		if(data.layanan[2] && data.type<4)cout << "        Poles   : " << Rupiah(biaya[data.type-1][2]) << endl;
		cout << endl<<"    Total       : " <<Rupiah(data.biaya)<< endl;
		writeLog(data);
		selesai.popPlat((char*)plat.c_str());
	}
}

void viewQueue() {
	/*
	 * menampilkan antrian dari tiap tempat pengerjaan
	 */
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
	/*
	 *  menampilkan catatan transaksi dari file dan total pemasukan
	 *	algo:
	 *		Buka file lalu baca isi file perbaris dan print
	 *      lalu tampilkan total transaksi
	 */
	system("CLS");
	cin.clear();
	cout << "-----------------------------------------------------------------------------------------------------------" << endl;
	cout << "|" << center("Daftar Transaksi", 105) << "|" << endl;
	cout << "-----------------------------------------------------------------------------------------------------------" << endl;
	cout << "|" << center("Kode", 12) << "|" << center("Mobil", 20) << "|" << center("Plat", 10) << "|" << center("Cuci", 7) << "|" << center("Vakum", 7) << "|" << center("Poles", 7) << "|" << center("Jam Masuk", 11) << "|" << center("Jam Keluar", 11) << "|" << center("Total", 12) << "|" << endl;
	cout << "-----------------------------------------------------------------------------------------------------------" << endl;
	ifstream logs(logFile);
	int total = 0;
	for (string line; getline(logs, line); )
	{
		string data[9];
		int count = 0;
		for (int x = 0; x < line.length(); x++) {
			if (line[x] == ',')count++;
			else data[count] += line[x];
		}
		total += stoi(data[8]);
		cout << "|" << center(data[0], 12) << "|" << center(data[1], 20) << "|" << center(data[2], 10) << "|" << center(data[3], 7) << "|" << center(data[4], 7) << "|" << center(data[5], 7) << "|" << center(data[6], 11) << "|" << center(data[7], 11) << "|" << center(Rupiah(stoi(data[8])), 12) << "|" << endl;
	}
	cout << "-----------------------------------------------------------------------------------------------------------" << endl;
	cout << "|" << right("Total ", 80) << "|" <<center( Rupiah(total),24) << "|" << endl;
	cout << "-----------------------------------------------------------------------------------------------------------" << endl;
}

int main()
{
	/*
	 *Entry point dari program Wash-Wash
	 */
	console.println(0, 0, center("WASH - WASH", 98));
	console.clearConsole();
	thread timer(ticker);
	console.goTo(0, 0);
	//print logo "WASH - WASH"
	cout << " __        ___    ____  _   _          __        ___    ____  _   _ " << endl;
	cout << " \\ \\      / / \\  / ___|| | | |         \\ \\      / / \\  / ___|| | | |" << endl;
	cout << "  \\ \\ /\\ / / _ \\ \\___ \\| |_| |  _____   \\ \\ /\\ / / _ \\ \\___ \\| |_| |" << endl;
	cout << "   \\ V  V / ___ \\ ___) |  _  | |_____|   \\ V  V / ___ \\ ___) |  _  |" << endl;
	cout << "    \\_/\\_/_/   \\_\\____/|_| |_|            \\_/\\_/_/   \\_\\____/|_| |_|" << endl;
	cout << endl;
	system("PAUSE");
	console.clearConsole();
	//menu
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
