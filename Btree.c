#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

const int KeyFieldMax = 12;
const int KFMaxPlus1 = 13;
const int DataFieldMax = 36;
const int DFMaxPlus1 = 37;
const int NULLCHAR = '\0';

typedef char KeyFieldType[13];
typedef char DataFieldType[37];

typedef struct {
	KeyFieldType KeyField;
	DataFieldType DataField;
} ItemType;

const int MaxKeys = 5;								// max number of keys in a node
const int MaxKeysPlusOne = 6;
const int MinKeys = 2;								// min number of keys in a node
const long NilPtr = -1L;							// the L indicates a long int

typedef struct {
	int Count;										// Number of keys stored in the current node
	ItemType Key[11];								// Warning: indexing starts at 0, not 1
	long Branch[12];								// Fake pointers to child nodes
} NodeType;

long Root;											// Pointer to the root node
long NumNodes;										// number of nodes in the B-tree
int NodeSize;										// number of bytes per node
NodeType CurrentNode;								// storage for current node being worked on
FILE *DataFile;										// the file stream for the table data
long NumItems;										// number of records of type ItemType in the table
char *OpenMode;										// r or w (read or write) mode for the table

void BTTable_constructor(char *, char *);
void BTTable_destructor();
int Insert(ItemType *);
int Retrieve(KeyFieldType, ItemType *);
int SearchNode(KeyFieldType, int *);
void AddItem(ItemType *, long, NodeType *, int);
void Split(ItemType *, long, long, int, ItemType *, long *);
void PushDown(ItemType *, long, int *, ItemType *, long *);
void Dump();

const int LineMax = 49;

void Error(char *);
int BT_make();
void Load(FILE *);
ItemType ReadLine(char *);
int BT_read();
bool ReadKeyboard(KeyFieldType);
void dump_helper();

int main()
{
	int cm;
	int cr;
	cm = BT_make();
	if (cm == 0) {
		Dump();
	}
	if (cm == 0) {
		cr = BT_read();
	}
	return 0;
}

void BTTable_constructor(char *Mode, char *FileName)
{
	OpenMode = Mode;
	NodeSize = sizeof(NodeType);
	if (Mode == "r") {
		// printf("constructor in read mode.\n");
		DataFile = fopen(FileName, "rb");
		if (!DataFile) {
			Error("File cannot be opened");
		}
		fread(&CurrentNode , NodeSize, 1, DataFile);
		if (!DataFile) {
			NumItems = 0;
			NumNodes = 0;
			Root = NilPtr;
		} else {
			NumItems = CurrentNode.Branch[0];
			NumNodes = CurrentNode.Branch[1];
			Root = CurrentNode.Branch[2];
		}
	} else if (Mode == "w") {
		// printf("constructor in write mode.\n");
		DataFile = fopen(FileName, "wb+");
		if (!DataFile) {
			Error("File cannot be opened");
		}
		Root = NilPtr;
		NumItems = 0;
		NumNodes = 0;
		CurrentNode.Branch[0] = NumItems;
		CurrentNode.Branch[1] = NumNodes;
		CurrentNode.Branch[2] = Root;
		fseek(DataFile, 0, SEEK_SET);
		fwrite(&CurrentNode, NodeSize, 1, DataFile);
	} else {
		Error("Incorrect mode given to BTTable constructor");
	}
}

void BTTable_destructor()
{
	if (OpenMode == "w") {
		CurrentNode.Branch[0] = NumItems;
		CurrentNode.Branch[1] = NumNodes;
		CurrentNode.Branch[2] = Root;
		fseek(DataFile, 0, SEEK_SET);
		fwrite(&CurrentNode, NodeSize, 1, DataFile);
	}
	fclose(DataFile);
}

void Error(char *msg)
{
	printf("%s\n", msg);
	return;
}

int BT_make()
{
	FILE *fout;
	BTTable_constructor("w", "btree.dat");
	fout = fopen("binput.dat", "r");
	if (!fout) {
		Error("ERROR: Unable to open file binput.dat");
		return 1;
	}
	Load(fout);
	fclose(fout);
	BTTable_destructor();
	return 0;
}
void Load(FILE *InputFile)
{
	ItemType Item;
	size_t len = 0;
	ssize_t read;
	char * Line = NULL;
	while ((read = getline(&Line, &len, InputFile)) != -1) {
		Item = ReadLine(Line);
		Insert(&Item);
		printf("%s %s\n", Item.KeyField, Item.DataField);
	}
}

ItemType ReadLine(char *Line)
{
	ItemType Item;
	KeyFieldType Word;
	DataFieldType Definition;
	int k, ch;
	for (k = 0; k < KeyFieldMax; k++) {
		Word[k] = Line[k];
	}
	Word[KeyFieldMax] = NULLCHAR;

	for (k = 0; k < DataFieldMax; k++) {
		ch = Line[KeyFieldMax + k];
		if (ch == '\n')
			break;
		Definition[k] = ch;
	}
	Definition[k] = NULLCHAR;
	strcpy(Item.KeyField, Word);
	strcpy(Item.DataField, Definition);
	return Item;
}

int BT_read()
{
	ItemType Item;
	KeyFieldType SearchKey;
	BTTable_constructor("r", "btree.dat");
	printf("Enter the word to be looked up (or . to quit): ");
	while (ReadKeyboard(SearchKey)) {
		if (Retrieve(SearchKey, &Item)) {
			printf(" Definition:\t%s\n", Item.DataField);
		} else {
			Error("Not found");
		}
		printf("Enter the word to be looked up (or . to quit): ");
	}
	return 0;
}

bool ReadKeyboard(KeyFieldType Word)
{
	int k;
	int ch;
	bool start;
	scanf("%[^\n]", Word);
	printf("%s\n", Word);
	if (Word[0] == '.') {
		return false;
	}
	start = true;
	for (k = 0; k < KeyFieldMax; k++) {
		ch = Word[k];
		if (ch == '\0') {
			start = false;
		}
		if (start) {
			Word[k] = toupper(ch);					// capitalize
		} else {
			Word[k] = ' ';							// pad with blanks
		}
	}
	Word[KeyFieldMax] = NULLCHAR;
	return true;
}


int Insert(ItemType *Item)
{
	// printf("Insert Called\n");
	int MoveUp;
	long NewRight;
	ItemType NewItem;
	PushDown(Item, Root, &MoveUp, &NewItem, &NewRight);

	if (MoveUp)
	{
		CurrentNode.Count = 1;
		CurrentNode.Key[0] = NewItem;
		CurrentNode.Branch[0] = Root;
		CurrentNode.Branch[1] = NewRight;
		NumNodes++;
		Root = NumNodes;
		fseek(DataFile, NumNodes * NodeSize, SEEK_SET);
		fwrite(&CurrentNode, NodeSize, 1, DataFile);
	}

	NumItems++;
	return 1;
}

int Retrieve(KeyFieldType SearchKey, ItemType *Item)
{
	// printf("Retrieve called.\n");
	long CurrentRoot;
	int Location;
	int Found;

	Found = 0;
	CurrentRoot = Root;
	printf("Go To Location of the Branch to go %ld\n", CurrentRoot);

	while ((CurrentRoot != NilPtr) && (! Found)) {
		fseek(DataFile, CurrentRoot * NodeSize, SEEK_SET);
		fread(&CurrentNode, NodeSize, 1, DataFile);
		if (SearchNode(SearchKey, &Location)) {
			Found = 1;
			*Item = CurrentNode.Key[Location];
			for (int i = 0; i <= Location; i++) {
				printf("%s", CurrentNode.Key[i].KeyField);
			}
			printf("\n");
		} else {
			for (int i = 0; i <= Location; i++) {
				printf("%s", CurrentNode.Key[i].KeyField);
			}
			printf("\n");
			CurrentRoot = CurrentNode.Branch[Location + 1];
			printf("Go To Location of the Branch to go %ld\n", CurrentRoot);
		}
	}
	return Found;
}


int SearchNode(KeyFieldType Target, int *Location)
{
	// printf("SearchNode called.\n");
	int Found;

	Found = 0;
	if (strcmp(Target, CurrentNode.Key[0].KeyField) < 0) {
		*Location = -1;
	} else {
		*Location = CurrentNode.Count - 1;
		while ((strcmp(Target, CurrentNode.Key[(*Location)].KeyField) < 0) && ((*Location) > 0)) {
			// printf("between %s %s\n", CurrentNode.Key[Location-1].KeyField, CurrentNode.Key[Location].KeyField);
			(*Location)--;
		}
		if (strcmp(Target, CurrentNode.Key[(*Location)].KeyField) == 0) {
			Found = 1;
		}
	}
	return Found;
}

void AddItem(ItemType *NewItem, long NewRight, NodeType *Node, int Location)
{
	// printf("AddItem called.\n");
	int j;

	for (j = (*Node).Count; j > Location; j--) {
		(*Node).Key[j] = (*Node).Key[j - 1];
		(*Node).Branch[j + 1] = (*Node).Branch[j];
	}

	(*Node).Key[Location] = (*NewItem);
	(*Node).Branch[Location + 1] = NewRight;
	(*Node).Count++;
}

void Split(ItemType *CurrentItem, long CurrentRight, long CurrentRoot, int Location, ItemType *NewItem, long *NewRight)
{
	// printf("split called\n");
	int j;
	int Median;
	NodeType RightNode;

	if (Location < MinKeys) {
		Median = MinKeys;
	} else {
		Median = MinKeys + 1;
	}

	fseek(DataFile, CurrentRoot * NodeSize, SEEK_SET);
	fread(&CurrentNode, NodeSize, 1, DataFile);
	for (j = Median; j < MaxKeys; j++) {
		RightNode.Key[j - Median] = CurrentNode.Key[j];
		RightNode.Branch[j - Median + 1] = CurrentNode.Branch[j + 1];
	}

	RightNode.Count = MaxKeys - Median;
	CurrentNode.Count = Median;

	if (Location < MinKeys) {
		AddItem(CurrentItem, CurrentRight, &CurrentNode, Location + 1);
	} else {
		AddItem(CurrentItem, CurrentRight, &RightNode, Location - Median + 1);
	}

	(*NewItem) = CurrentNode.Key[CurrentNode.Count - 1];
	RightNode.Branch[0] = CurrentNode.Branch[CurrentNode.Count];
	CurrentNode.Count--;

	fseek(DataFile, CurrentRoot * NodeSize, SEEK_SET);
	fwrite(&CurrentNode, NodeSize, 1, DataFile);
	NumNodes++;
	(*NewRight) = NumNodes;
	fseek(DataFile, (*NewRight) * NodeSize, SEEK_SET);
	fwrite(&RightNode, NodeSize, 1, DataFile);
}

void PushDown(ItemType *CurrentItem, long CurrentRoot, int *MoveUp, ItemType *NewItem, long *NewRight)
{
	// printf("PushDown called.\n");
	int Location;
	if (CurrentRoot == NilPtr) {
		(*MoveUp) = 1;
		NewItem = CurrentItem;
		(*NewRight) = NilPtr;
	} else {
		fseek(DataFile, CurrentRoot * NodeSize, SEEK_SET);
		fread(&CurrentNode, NodeSize, 1, DataFile);
		if (SearchNode((*CurrentItem).KeyField, &Location)) {
			Error("Error: attempt to put a duplicate into B-tree");
		}
		PushDown(CurrentItem, CurrentNode.Branch[Location + 1], MoveUp, NewItem, NewRight);
		if ((*MoveUp)) {
			fseek(DataFile, CurrentRoot * NodeSize, SEEK_SET);
			fread(&CurrentNode, NodeSize, 1, DataFile);
			if (CurrentNode.Count < MaxKeys) {
				(*MoveUp) = 0;
				AddItem(NewItem, (*NewRight), &CurrentNode, Location + 1);
				fseek(DataFile, CurrentRoot * NodeSize, SEEK_SET);
				fwrite(&CurrentNode, NodeSize, 1, DataFile);
			} else {
				(*MoveUp) = 1;
				Split(NewItem, (*NewRight), CurrentRoot, Location, NewItem, NewRight);
			}
		}
	}
}

void Dump()
{
	int k;
	long p;
	printf("Root is node (record) number %ld\n", Root);

	for (p = 0; p <= NumNodes; p++) {
		fseek(DataFile, p * NodeSize, SEEK_SET);
		fread(&CurrentNode, NodeSize, 1, DataFile);

		if (p == 0) {
			printf("Node 0 is not part of tree, contains this data:\n");
			printf("\tNumItems = %ld\n", CurrentNode.Branch[0]);
			printf("\tNumNodes = %ld\n", CurrentNode.Branch[1]);
			printf("\tRoot = %ld\n", CurrentNode.Branch[2]);
		} else {
			printf("\t\tDump of node number = %ld\n", p);
			printf("\t\tCount : %d\n", CurrentNode.Count);
			printf("\t\tKeys: ");

			for (k = 0; k < CurrentNode.Count; k++) {
				printf("%s ", CurrentNode.Key[k].KeyField);
			}

			printf("\n\t\tBranches: ");
			for (k = 0; k <= CurrentNode.Count; k++) {
				printf("%ld ",  CurrentNode.Branch[k] );
			}
			printf("\n");
		}
	}
	return;
}