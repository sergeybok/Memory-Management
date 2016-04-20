#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;


// Constants -----------------------------------------------------
/*
template<std::size_t N>
bool operator<(const std::bitset<N>& x, const std::bitset<N>& y)
{
    for (int i = N-1; i >= 0; i--) {
        if (x[i] ^ y[i]) return y[i];
    }
    return false;
}
*/

bool bitlessthan(std::bitset<32> b1, std::bitset<32> b2) {
	for (int i =31; i > -1; i--) {
		if (b1[i] == b2[i]) {
			continue;
		}
		if (b1[i] < b2[i]) {
			return true;
		}
		if (b1[i] > b2[i]) {
			return false;
		}
	}
	return false;
}

static ifstream file;
static int randoms_counter;
std::vector<int> randoms;

//std::vector<PTE *> pagetable(64);
//std::vector<Frame *> frametable;


void read_randoms(string rand_file) {
	file.open(rand_file);
	string w;
	randoms_counter = 0;
	while(file>>w) {
		randoms.push_back(std::stoi(w));
	}
	file.close();
}

int myrandom(int burst) {
	if (randoms_counter == randoms.size()) randoms_counter = 0;
	return (randoms[++randoms_counter] % burst);
	//return (randoms[randoms_counter++] % burst);
}


// Classes -----------------------------------------------

class PTE {
public:
	// Referenced
	unsigned reference:1;
	// Valid == Present
	unsigned valid:1;
	//Modified
	unsigned modified:1;
	// Pagedout???
	unsigned pageout:1;
	unsigned frame:28;

	PTE() {
		reference = 0;
		valid = 0;
		modified = 0;
		pageout = 0;
	}
	void reinit() {
		reference = 0;
		valid = 0;
		modified = 0;
	}

	void read() {
		reference = 1;
	}
	void write() {
		modified = 1;
	}

};


class Frame {
public:
	PTE *pte;
	int pid;
	Frame() {
		pte = nullptr;
	}

	void setPTE(PTE *p) {
		pte = p;
	}
};

class Mem_Manager {
public:
	std::vector<Frame *> *frametable;
	std::vector<PTE *> *pagetable;
	
	void setFT(std::vector<Frame *> *ft) {
		frametable = ft;
	}
	void setPT(std::vector<PTE *> *pt) {
		pagetable = pt;
	}

	virtual Frame * allocate_frame() = 0;
	virtual void update(PTE *pte, Frame *f) = 0;

	virtual void print_pages() {

		for (int i =0; i< pagetable->size(); i++) {
			if (pagetable->at(i)->valid == 0) {
				if (pagetable->at(i)->pageout == 1) {
					cout << "# ";
				} else {
					cout << "* ";
				}
				continue;
			}
			cout << i<<":";
			if (pagetable->at(i)->reference == 1) {
				cout << "R";
			} 
			else {
				cout << "-";
			}
			if (pagetable->at(i)->modified == 1) {
				cout << "M";
			}
			else {
				cout << "-";
			}
			if (pagetable->at(i)->pageout == 1) {
				cout << "S ";
			}
			else {
				cout << "- ";
			}
		}	
		cout << endl;
		
	}

	virtual void print_frame() {
		PTE *p;
		int page;
		for (int i =0; i< frametable->size(); i++) {
			p = frametable->at(i)->pte;
			if (p == nullptr) {
				cout << "* ";
			}
			else {
				page = find(pagetable->begin(), pagetable->end(), p) - pagetable->begin();
				cout << page << " ";
			}
		}
		cout << endl;
	}
	
};

class NRU : public Mem_Manager {
public:
	int count;

	NRU() {
		count = 0;
	}

	Frame * allocate_frame() {
		count++;

		std::vector<PTE *> class0;
		std::vector<PTE *> class1;
		std::vector<PTE *> class2;
		std::vector<PTE *> class3;
		PTE *p;
		for (int i = 0; i< pagetable->size(); i++) {
			p = pagetable->at(i);
			if (pagetable->at(i)->valid == 1) {
				if (p->reference == 0 && p->modified == 0) {
					class0.push_back(p);
				}
				else if (p->reference == 0 && p->modified ==1){
					class1.push_back(p);
				}
				else if (p->reference == 1 && p->modified ==0){
					class2.push_back(p);
				}
				else if (p->reference ==1 && p->modified ==1){
					class3.push_back(p);
				}
			}
		}

		Frame *f;
		int r,fi;
		if (!class0.empty()) {
			r = myrandom(class0.size());
			fi = class0[r]->frame;
			f = frametable->at(fi);
		}
		else if (!class1.empty()) {
			r = myrandom(class1.size());
			fi = class1[r]->frame;
			f = frametable->at(fi);
		}
		else if (!class2.empty()) {
			r = myrandom(class2.size());
			fi = class2[r]->frame;
			f = frametable->at(fi);
		}
		else if (!class3.empty()) {
			r = myrandom(class3.size());

			fi = class3[r]->frame;
			f = frametable->at(fi);
		}
		class0.clear();
		class1.clear();
		class2.clear();
		class3.clear();
		return f;
	}

	void update(PTE *pte, Frame *f) {
		//if (false){
		if (count == 10) {
			// 
			PTE *p;
			for (int i = 0; i< pagetable->size(); i++){
				if (pte == pagetable->at(i)) {
					continue;
				}
				p = pagetable->at(i);
				//p->modified = 0;
				p->reference = 0;
			}
			count = 0;
		}
		//count++;
	}
	
};

class LRU : public Mem_Manager {
public:
	std::vector<std::vector<bool> > count;

	LRU(int num_frame) {
		for (int i=0; i<num_frame; i++) {
			count.push_back(std::vector<bool>(num_frame));
		}
		for (int i=0; i<num_frame; i++) {
			for (int j=0; j<num_frame; j++) {
				count[i][j] = false;
			}
		}
	}

	Frame * allocate_frame() {
		PTE *p;
		Frame *f;
		int min = 1000;
		int index;
		int row_sum;
		for (int i =0; i<count.size(); i++) {
			row_sum = 0;
			for (int j=0; j< count.size(); j++) {
				if (count[i][j]) {
					row_sum++;
				}
			}
			if (row_sum < min) {
				index = i;
				min = row_sum;
			}
		}
		f = frametable->at(index);
		return f;
	}

	void update(PTE *pte, Frame *f) {
		//
		for (int i=0; i < count.size(); i++) {
			for (int j=0; j<count.size(); j++) {
				if (j == pte->frame) {
					count[i][j] = false;
					continue;
				}
				if (i == pte->frame) {
					count[i][j] = true;
				}
			}
		}
	}
};

class RandMMU: public Mem_Manager {
public:

	Frame * allocate_frame() {
		int r = myrandom(frametable->size());
		return frametable->at(r);
	}
	void update(PTE *pte, Frame *f) {
		
	}
};

class FIFO : public Mem_Manager { // ============================
public:

	std::vector<Frame *> queue;

	Frame * allocate_frame() {
		if (queue.empty()) {
			return nullptr;
		}
		Frame *f = queue.front();
		queue.erase(queue.begin());
		return f;
	}
	void update(PTE *pte, Frame *f) {
		if (f == nullptr) {
			return;
		}
		if (std::find(queue.begin(),queue.end(),f) != queue.end()) {
			queue.erase(find(queue.begin(),queue.end(),f));
		}
		queue.push_back(f);
	}
};

class SecondChance : public Mem_Manager {
public:

	std::vector<Frame *> queue;

	Frame * allocate_frame() {
		if (queue.empty()) { return nullptr; }
		PTE *p;
		Frame *f;
		while (true){
			f = queue.front();
			queue.erase(queue.begin());
			p = f->pte;
			if (p->reference == 0) {
				break;
			}
			p->reference = 0;
			queue.push_back(f);
		}
		return f;
	}
	void update(PTE *pte, Frame *f) {
		if (f == nullptr) {
			return;
		}
		if (std::find(queue.begin(),queue.end(),f) != queue.end()) {
			queue.erase(find(queue.begin(),queue.end(),f));
		}
		queue.push_back(f);
	}
};

class Clock_phys : public Mem_Manager {
public:
	std::vector<Frame *> queue;
	int hand;
	int begin;

	Clock_phys(int num_frame) {
		hand = 0;
		begin = 0;
		for (int i =0; i< num_frame; i++) {
			queue.push_back(nullptr);
		}
	}
	
	Frame * allocate_frame() {
		if (queue.empty()) {return nullptr;}
		PTE *p;
		Frame *f;
		while (true) {
			f = queue[hand];
			p = f->pte;
			if (p->reference == 0) {
				break;
			}
			p->reference = 0;
			if ((hand + 1) >= queue.size()) {hand = 0;}
			else {hand++;}
		}
		queue[hand] = nullptr;
		if (hand +1 >= queue.size() ) {
			hand = 0;
		}
		else { hand++;}
		
		return f;
	}

	void update(PTE *pte, Frame *f) {
		if (f == nullptr) {
			return;
		}

		if (begin < queue.size()) {
			queue[begin] = f;
			begin++;
			return;
		}

		if (hand == 0) {
			//cout << "insert: " << f->pid << endl;
			if (queue[queue.size()-1] != nullptr) {
				cout << "problem!!!" << endl;
				exit(1);
			}
			queue[queue.size()-1] = f;
		}
		else {
			//cout << "insert: " << f->pid << endl;
			if (queue[hand-1] != nullptr) {
				cout << "hand: " << hand <<endl;
				cout << queue[hand]->pid;
				cout << " problem" << endl;
				exit(1);
			}
			queue[hand-1] = f;
		}
	}
	/*
	void print_pages() {
		for (int i =0; i< pagetable->size(); i++) {
			if (pagetable->at(i)->valid == 0) {
				if (pagetable->at(i)->pageout == 1) {
					cout << "# ";
				} else {
					cout << "* ";
				}
				continue;
			}
			cout << i<<":";
			if (pagetable->at(i)->reference == 1) {
				cout << "R";
			} 
			else {
				cout << "-";
			}
			if (pagetable->at(i)->modified == 1) {
				cout << "M";
			}
			else {
				cout << "-";
			}
			if (pagetable->at(i)->pageout == 1) {
				cout << "S ";
			}
			else {
				cout << "- ";
			}
		}
		cout << endl;
	}

	void print_frame() {
		PTE *p;
		int page;
		for (int i =0; i< frametable->size(); i++) {
			p = frametable->at(i)->pte;
			if (p == nullptr) {
				cout << "* ";
			}
			else {
				page = find(pagetable->begin(), pagetable->end(), p) - pagetable->begin();
				cout << page << " ";
			}
		}
		cout << " || hand = " << hand << endl;;
	}
	*/
};

class Clock_virt : public Mem_Manager {
public:
	int hand;
	//
	Clock_virt(int num_f) {
		hand = 0;
	}

	Frame * allocate_frame() {
		//if (queue.empty()) {return nullptr;}
		PTE *p;
		Frame *f;
		while (true) {
			p = pagetable->at(hand);
			if (p->valid == 1 && p->reference == 0){
				f = frametable->at(p->frame);
				break;
			}
			p->reference = 0;
			if (hand+1 >= 64) {
				hand = 0;
			}
			else { hand++;}
		}
		if (hand+1 >= 64) {
			hand = 0;
		}
		else {hand++;}
		return f;
		
	}
	void update(PTE *pte, Frame *f) {
		
	}
	/*
	void print_pages() {
		for (int i =0; i< pagetable->size(); i++) {
			if (pagetable->at(i)->valid == 0) {
				if (pagetable->at(i)->pageout == 1) {
					cout << "# ";
				} else {
					cout << "* ";
				}
				continue;
			}
			cout << i<<":";
			if (pagetable->at(i)->reference == 1) {
				cout << "R";
			} 
			else {
				cout << "-";
			}
			if (pagetable->at(i)->modified == 1) {
				cout << "M";
			}
			else {
				cout << "-";
			}
			if (pagetable->at(i)->pageout == 1) {
				cout << "S ";
			}
			else {
				cout << "- ";
			}
		}	
		cout << endl;
		

	}
	void print_frame() {
		PTE *p;
		int page;
		for (int i =0; i< frametable->size(); i++) {
			p = frametable->at(i)->pte;
			if (p == nullptr) {
				cout << "* ";
			}
			else {
				page = find(pagetable->begin(), pagetable->end(), p) - pagetable->begin();
				cout << page << " ";
			}
		}
		cout << " || hand = " << hand << endl;;
	}
	*/
};

class Aging_phys : public Mem_Manager {
public:
	std::vector<bitset<32> > count;

	Aging_phys(int num_frame) {
		for (int i =0; i< num_frame; i++) {
			bitset<32> b;
			count.push_back(b);
		}
 	}

	Frame * allocate_frame() {
		Frame *f;
		bitset<32> max;
		max = max.flip();
		
		int index = -1;
		PTE *p;
		for (int i =0; i<count.size(); i++) {
			p = frametable->at(i)->pte;
			//
			bitset<32> b = p->reference<<31;
			count[i] = b | (count[i]>>1);
			//
			//if (count[i] < max){
			if (bitlessthan(count[i],max)){
				max = count[i];
				index = i;
				f = frametable->at(i);
			}
			//count[i][31] = 1;
			p->reference = 0;
		}
		count[index].reset();

		return f;
	}
	void update(PTE *pte, Frame *f) {
		
	}
};

class Aging_virt : public Mem_Manager {
public:

	std::vector<bitset<32> > count;

	Aging_virt() {
		for (int i =0; i<64; i++) {
			bitset<32> b;
			count.push_back(b);
		}
	}

	Frame * allocate_frame() {
		Frame *f;
		bitset<32> max;
		max = max.flip();
		
		int index = -1;
		PTE *p;
		for (int i =0; i<count.size(); i++) {
			p = pagetable->at(i);
			if (p->valid == 0) {
				continue;
			}
			//
			bitset<32> b = p->reference<<31;
			count[i] = b | (count[i]>>1);
			//
			//if (count[i] < max){
			if (bitlessthan(count[i],max)){
				max = count[i];
				index = i;
				f = frametable->at(p->frame);
			}
			//count[i][31] = 1;
			p->reference = 0;
		}
		count[index].reset();

		return f;
	}
	void update(PTE *pte, Frame *f) {
		
	}
};






// Functions -----------------------------------------------------



Mem_Manager * setMMU(char *argv, int i) {
	if (argv[2] == 'N') {
		// NRU
		
		return new NRU();
	}
	else if(argv[2] == 'l') {
		// LRU
		return new LRU(i);
	}
	else if (argv[2] == 'r') {
		// RAndom
		return new RandMMU();
	}
	else if(argv[2] == 'f') {
		// FIFO
		return new FIFO();
	}
	else if(argv[2] == 's') {
		// Second chance
		return new SecondChance();
	}
	else if (argv[2] == 'c') {
		// Clock physical
		return new Clock_phys(i);
	}
	else if (argv[2] == 'X') {
		// Clock virtual
		return new Clock_virt(i);
	}
	else if (argv[2] == 'a') {
		// Aging phys
		return new Aging_phys(i);
	}
	else if(argv[2] == 'Y') {
		// Aging virtual
		return new Aging_virt();
	}
	return nullptr;
}

Frame * get_frame(Mem_Manager *mmu, std::vector<Frame *> ft) {
	for (int i = 0; i< ft.size(); i++) {
		if (ft[i]->pte == nullptr) {
			return ft[i];
		}
	}
	return mmu->allocate_frame();
}

void map(int page, Frame *f, int instruction, std::vector<PTE *> pagetable, bool O) {
	f->pte = pagetable[page];
    pagetable[page]->valid = 1;
    pagetable[page]->frame = f->pid;
    if (O) {
    	cout << instruction <<": MAP\t"<<page<<"\t"<<(f->pid)<<endl;	
    }
}




// Main ---------------------------------------------------------

int main(int argc, char* argv[]) {
	std::vector<char> option;
	string infile;
	string rfile;
	int num_frame;
	Mem_Manager *mmu;

	bool O = false;
	bool P = false;
	bool F = false;
	bool S = false;


	// Get args from command line
	int i = 1;
	int al=2;

	while (argv[i][0] == '-') {
		if (argv[i][1] == 'a'){
			al = i;
		}
		if (argv[i][1] == 'o'){
			for (int j =2; j<strlen(argv[i]); j++){
				if (argv[i][j] == 'O') {
					O = true;
				}
				if (argv[i][j] == 'P') {
					P = true;
				}
				if (argv[i][j] == 'F') {
					F = true;
				}
				if (argv[i][j] == 'S') {
					S = true;
				}
			}
		}
		if (argv[i][1] == 'f') {
			int l = strlen(argv[3]);
			string q;
			for (int j=2;j<l; j++) {
				q += argv[i][j];
			}
			num_frame = stoi(q);
		}
		i++;
	}

	mmu = setMMU(argv[al],num_frame);

	/*
	int l = strlen(argv[3]);
	string q;
	for (int j=2;j<l; j++) {
		q += argv[3][j];
	}
	num_frame = stoi(q);

	int i = 1;
	mmu = setMMU(argv[i], num_frame);
	i++;

	// Options
	
	for (int j =2; j<strlen(argv[i]); j++){
		if (argv[i][j] == 'O') {
			O = true;
		}
		if (argv[i][j] == 'P') {
			P = true;
		}
		if (argv[i][j] == 'F') {
			F = true;
		}
		if (argv[i][j] == 'S') {
			S = true;
		}
	}
	i++;
	i++;
	
	*/
	//input file
	infile = argv[i];
	i++;

	// Rand file
	rfile = argv[i];
	read_randoms(rfile);


	// initialize Pagetable and Frametable
	std::vector<PTE *> pagetable(64);
	for (int i =0; i < pagetable.size(); i++) {
		pagetable[i] = new PTE();
	}
	std::vector<Frame *> frametable(num_frame);
	for (int i =0; i < num_frame; i++) {
		frametable[i] = new Frame();
		frametable[i]->pid = i;
	}

	mmu->setPT(&pagetable);
	mmu->setFT(&frametable);

	//counters
	unsigned int unmaps=0;
	unsigned int maps=0;
	unsigned int ins=0;
	unsigned int outs=0;
	unsigned int zeros=0;


	//Start reading file
	std::string line;
	std::ifstream inf(infile);
	unsigned int instruction = 0;

	
	while (std::getline(inf, line)) {
    	std::istringstream iss(line);
    	int op, page;
    	if (!(iss >> op >> page)) { continue; }

    	if (O) {
    		cout << "==> inst: "<<op<<" "<<page<<endl;
    	}
  		Frame *frame = nullptr;
    	if (pagetable[page]->valid == 0) {
 			//pagetable[page]->reference = 1;
    		frame = get_frame(mmu,frametable);
    		int frame_i = frame->pid;
    		
    		if (frame->pte != nullptr) {
    			PTE *out = frame->pte;
    			if (out->modified == 1) {
    				out->pageout = 1;
    			}
    			int out_i = find(pagetable.begin(),pagetable.end(),out) - pagetable.begin();
    			//
    			if (O) {
    				cout << instruction<<": UNMAP\t"<<out_i<<"\t"<<frame_i<<endl;	
    			}
    			unmaps++;
    			if (out->modified == 1) {
    				if (O) {
    					cout << instruction<<": OUT\t"<<out_i<<"\t"<<frame_i<<endl;	
    				}
    				outs++;
    			}
    			if (pagetable[page]->pageout == 1) {
    				//pagetable[page]->pageout = 0;
    				if (O) {
    					cout << instruction<<": IN  \t"<<page<<"\t"<<frame_i<<endl;	
    				}
    				ins++;
    			}
    			else {
    				if (O) {
    					cout << instruction<<": ZERO\t\t"<<frame_i<<endl;	
    				}
    				zeros++;
    			}
    			out->reinit();
    		}
    		else {
    			// zero(frame)
    			if (O) {
    				cout << instruction<<": ZERO\t\t"<<frame_i<<endl;	
    			}
    			zeros++;
    		}
    		// Map
    		map(page, frame, instruction,pagetable,O);
    		maps++;
    	}

    	//cout << "TEST after"<<endl;
    	// Operation
    	if (op == 0){
    		pagetable[page]->reference = 1;
    	} if (op == 1) {
    		pagetable[page]->modified = 1;
    		pagetable[page]->reference = 1;
    	}
    	//
    	mmu->update(pagetable[page],frame);
    	instruction++;
    	//mmu->print_pages();
	}

	// SUM
	unsigned long long totalcost=0;
	totalcost += instruction*1; //each instruction has r/w, cost 1 cycle
	totalcost += zeros*150; //zeroing costs 150
	totalcost += ins*3000; // ins and outs cost 3000
	totalcost += outs*3000;
	totalcost += maps*400; //maps and unmaps cost 400
	totalcost += unmaps*400;

	if (P) {
		mmu->print_pages();
	}

	if (F) {
		mmu->print_frame();
	}

	if (S) {
		printf("SUM %d U=%d M=%d I=%d O=%d Z=%d ===> %llu\n",
			instruction,unmaps,maps,ins,outs,zeros,totalcost);
	}




	return 0;
}



