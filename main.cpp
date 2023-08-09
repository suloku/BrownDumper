#include <stdio.h>
#include <string.h>
#include <cstdint>
#include <stdlib.h>

#include "constants.h"

//#define DEBUG

//Tracker for output formatting
//Output "browndumper.exe -dumpcsv -moves rom.gb >> moves.csv" to get a move list in a CSV file format
int comadump = 0;

//Defining this will swap front and back pics offsets in the rom
//#define SWAP_PICS

#define CHECK_BIT(var,pos) ((var>>pos) & 1)

//Methods
uint16_t swap_uint16( uint16_t val );
int ThreeByteToTwoByte (int bank, uint16_t off);
int ThreeByteToTwoByteBswap (int bank, uint16_t off);
int getBank (int off);
int TwoByteOffset (int off);

void my_exit();
void ProcessBasestats (char*buffer);
void ProcessEvoMoves (char*buffer);
void ProcessMoveData (char*buffer);
void ProcessWildData (char*buffer);
void ProcessHiddenData (char*buffer, int offset);
void ProcessTypeTableData (char*buffer);
void ProcessTrainerData (char*buffer);

//Constants
    int totalmon = 255;
    //Base stats
        int BasesstatsOffset = 0xFC336;
        int basestat_size = 28;
    //Evolutions and learnsets
        int EvomovesPointerTableOffset = 0x3bdf9;//0e:7df9
        int evomovebank = 0xE;
    //Move data
        int CriticalListOffset = 0x3e540;
        int MoveDataOffset = 0x3B250;
        int move_size = 6;
        int totalmoves = 256;
        int PSsplitDataOffset = 0x3ffb0;//Phisical/Special moves are stored 3ffb0-3ffCf (32 bytes, 256 flags)
    //Wild data
        int totalmaps = 247; //Start at map 0
        int WildPointersOffset = 0xCEEB;
        uint16_t curWildOffset = 0;
    //Hidden data
        int HiddenObjectMapsOffset = 0x46a40;
        int HiddenObjectPointersOffset = 0x46a96;
        int hiddenObjectsBank = 0x11; //Hidden object data is at bank 17
        int totalHiddenMaps = 84; //they are stored contiguously, each map is 1 byte
        int curHiddenOffset = 0;
        uint16_t HiddenOffsetU16 = 0;
    //Types table
        int TypeEffectivenesOffset = 0x3fBC8;



//Work Mode constants
#define NO_DUMP 0
#define DUMP_STATS 1
#define DUMP_MOVE 2
#define DUMP_WILD 3
#define DUMP_HIDDEN 4
#define DUMP_TYPETABLE 5
#define DUMP_TRAINERS 6

char * romBuffer;
int main (int argc, char** argv)
{

    int mode = NO_DUMP;

    printf("\n");
    printf("------------------------------------------------------");
    printf("\nPok�mon Brown Data Dumper 1.0 by suloku 2023\n");
    printf("------------------------------------------------------");
    printf("\n\n");

    if (argc > 4 || argc < 3 || (argc == 4 && (strcmp(argv[1], "-dumpcsv")!=0)) )
    {
	    if (argc < 3) printf("\nToo few arguments.\n");
	    if (argc == 4 && strcmp(argv[1], "-dumpcsv")!= 0) printf ("\n%s is not a valid command.\n", argv[1]);
	    if (argc > 4) printf("\nToo many arguments.\n");
exit_message:
		printf("\nUsage:\n%s (-dumpcsv) -[stats|moves|wild|hidden|typestable] ROM%d", argv[0], argc);
		printf("\n\t-dumpcsv:  dump as CSV format (optional)");
		printf("\n\t-stats:  dump base stats, evolution data and learnsets");
		printf("\n\t-moves:  dump move data");
		printf("\n\t-wild:   dump wild pok�mon data");
		printf("\n\t-hidden: dump hidden item data (only items, excludes hidden objects)");
		printf("\n\t-typestable: dump type effectiveness table (all combinations not in table are neutral damage)");
		printf("\n\t-trainers: dump trainer teams based on trainer class");
        my_exit();
	}
	int current_argument = 1;
    if( !strcmp(argv[current_argument], "-dumpcsv") )
	{
        comadump = 1;
        current_argument++;
	}

	if( !strcmp(argv[current_argument], "-stats") )
	{
        mode = DUMP_STATS;
	}
	else if( !strcmp(argv[current_argument], "-moves") )
	{
        mode = DUMP_MOVE;
	}
	else if( !strcmp(argv[current_argument], "-wild") )
	{
        mode = DUMP_WILD;
	}
	else if( !strcmp(argv[current_argument], "-hidden") )
	{
        mode = DUMP_HIDDEN;
	}
	else if( !strcmp(argv[current_argument], "-typestable") )
	{
        mode = DUMP_TYPETABLE;
	}
    else if( !strcmp(argv[current_argument], "-trainers") )
	{
        mode = DUMP_TRAINERS;
	}
	else
    {
        printf("\n%s is not a valid command.\n", argv[current_argument]);
		goto exit_message;
	}


//Open rom

  FILE * pFile;
  long lSize;
  size_t result;

  pFile = fopen ( argv[current_argument+1] , "rb" );
  if (pFile==NULL) {fputs ("File error",stderr); exit (1);}

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);
  lSize = ftell (pFile);
  rewind (pFile);

  // allocate memory to contain the whole file:
  romBuffer = (char*) malloc (sizeof(char)*lSize);
  if (romBuffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  result = fread (romBuffer,1,lSize,pFile);
  if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  /* the whole file is now loaded in the memory buffer. */

  // terminate
  fclose (pFile);

//ROM Loaded

//Process Base Stats
    if (mode == DUMP_STATS)
    {
        if (comadump)
        {
            //printf("Species;Dexnumber;Name;HP;ATK;DEF;SPD;SPC;Type1;Type2;CatchRate;BaseExp;Move1;Move2;Move3;Move4;GrowthRate;SpriteDim;PicFront;PicBack;PicBank;");
            printf("Species;Dexnumber;Name;HP;ATK;DEF;SPD;SPC;Type1;Type2;CatchRate;BaseExp;Move1;Move2;Move3;Move4;GrowthRate;Learnset;");
            for (int j =0; j<55; j++)
            {
                printf("%s;", TM_HM[j]);
            }
            printf("Learnset;");
            printf("\n");
        } // COMADUMP

        for (int i =0; i<totalmon; i++)
        {
            //printf("Entry %03d Address: 0x%X\n", i, BasesstatsOffset+i*basestat_size);
            //ProcessBasestats(romBuffer+BasesstatsOffset+i*basestat_size);
        }

        //Process Evo-move data
        uint16_t u16evomovePointer = 0;
        int movepointer = 0;
        int dexnum;

        for (int i =0; i<=totalmon; i++)
        {
            dexnum = data2dex[i+1];
            if (comadump)
            {
                printf("%03d;", i);
            }
            else
            {
                printf("Index: %03d ", i);
                //printf ("%d", dexnum);
                printf("(0x%X) ", BasesstatsOffset+((dexnum-1)*basestat_size));
            }

            ProcessBasestats(romBuffer+BasesstatsOffset+((dexnum-1)*basestat_size));
            memcpy(&u16evomovePointer, (romBuffer+EvomovesPointerTableOffset+i*2), 2);
            if (!u16evomovePointer)
            {
                if (!comadump)
                {
                    printf("Evomove pointer 0x0000, no entry (decamark entry?)");
                }
            }
            else
            {
                movepointer = ThreeByteToTwoByte(evomovebank, u16evomovePointer);

                if (!comadump)
                {
                    //printf("0x%X (%X:%X)\n", movepointer, evomovebank, u16evomovePointer);
                    //printf("\n%03d %s", i, SpeciesName[i+1]);
                    printf(" EvoMoves offset 0x%X (pointer at 0x%X)", movepointer, EvomovesPointerTableOffset+i*2);
                }
                ProcessEvoMoves(romBuffer+movepointer);
            }
            printf("\n");
            if (!comadump)
            {
                printf("\n");
            }
        }
    }
//MOVE DATA
    else if (mode == DUMP_MOVE)
    {
        //Dump increased crit chance move list
        if (!comadump)
        {
            printf("Increased critical hit chance move list:\n");
        }
        else
        {
            printf("Increased critical hit chance;");
        }

        int cursor = 0;
        while (1)
        {
            if ((uint8_t) romBuffer[CriticalListOffset+cursor] == 0xFF) break;
            if (!comadump)
            {
                printf("\t%03d %s\n", (uint8_t)romBuffer[CriticalListOffset+cursor], MoveNames[(uint8_t)romBuffer[CriticalListOffset+cursor]]);
            }
            else
            {
                printf("%03d %s;", (uint8_t)romBuffer[CriticalListOffset+cursor], MoveNames[(uint8_t)romBuffer[CriticalListOffset+cursor]]);
            }
            cursor ++;
        }

        if (comadump)
        {
            printf("\n");
        }

        //Process Move Data (skip move with index 0x00, a glitch move since there's no move data associated)
        //Remember MoveDataOffset is the start of move data minus 6 bytes to account for this fact
        //MoveDataOffset = MoveDataOffset-move_size;

        if (comadump)
        {
            printf("Index;Name;Type;Power;Accuracy;PP;Effect;Anim;P/S;Address;\n");
        }

        for (int i =1; i<totalmoves; i++)
        {
            //Move names are hardcoded and should be read from the rom, but that would need a character encoding conversion
        if (!comadump)
        {
            printf("\nMove %03d %s (Address: 0x%X)\n", i, MoveNames[i], MoveDataOffset);
        }
        else
        {
            printf("%03d;%s;", i, MoveNames[i]);
        }
            ProcessMoveData(romBuffer+MoveDataOffset);

            //Process PS split bitflags (1 flag for each move, 256 flags, 32 bytes)
            //Get the byte were the move is stored
            int curbyte = (i-1)/8;
            //Get the bit in that byte
            int curbit = (i-1)-(curbyte*8);
            uint8_t ps_byte = romBuffer[PSsplitDataOffset+curbyte];
            //Test the bit
            if (CHECK_BIT(ps_byte, curbit))
            {
                if (!comadump)
                {
                        printf("\t P/S: Special\n");
                }
                else
                {
                        printf("Special;");
                }
            }
            else
            {
                if (!comadump)
                {
                        printf("\t P/S: Physical\n");
                }
                else
                {
                        printf("Physical;");
                }
            }

            if (comadump)
            {
                printf("0x%X;\n", MoveDataOffset);
            }
            MoveDataOffset += move_size;
        }
    }
//WILD Pokemom DATA
    else if (mode == DUMP_WILD)
    {
        for (int i =0; i<=totalmaps; i++)
        {
            //Read wild encounter data for all maps7
            // Read two byte pointer to wild data (wild data is stored in bank 0x03)
            memcpy(&curWildOffset, romBuffer+WildPointersOffset+(2*i), sizeof(curWildOffset));
            //Convert offset to absolute offset
            curWildOffset = ThreeByteToTwoByte(0x03, curWildOffset);

           // Read wild data
           printf("Map %2d %s (Wild data offset: 0x%04X, %02X:%04X) (wild pointer offset 0x%04X, %02X:%04X)\n", i, MapNames[i], curWildOffset, getBank(curWildOffset), TwoByteOffset(curWildOffset), WildPointersOffset+(2*i), getBank(WildPointersOffset+(2*i)), TwoByteOffset(WildPointersOffset+(2*i)));
           ProcessWildData(romBuffer+curWildOffset);

        }//all maps read
    }
//Hidden item
    else if (mode == DUMP_HIDDEN)
    {
        for (int i =0; i<=totalHiddenMaps; i++)
        {

            // Read two byte pointer to current map hidden object data
            memcpy(&HiddenOffsetU16, romBuffer+HiddenObjectPointersOffset+(2*i), 2);

            //Convert offset to absolute offset
            curHiddenOffset = ThreeByteToTwoByte(hiddenObjectsBank, HiddenOffsetU16);

           // Read hidden data
           //printf("%i: Map %s (0x%02X) (Hidden data offset: 0x%04X) (Pointer at: 0x%04X)\n", i, MapNames[(uint8_t)(romBuffer[HiddenObjectMapsOffset+i])], (uint8_t)romBuffer[HiddenObjectMapsOffset+i], curHiddenOffset, HiddenObjectPointersOffset+(2*i));
            printf("%i: ", i);
            printf("Map %s ", MapNames[(uint8_t)(romBuffer[HiddenObjectMapsOffset+i])]);
            //printf("(0x%02X) ", (uint8_t)romBuffer[HiddenObjectMapsOffset+i]);
            printf("(Hidden data offset: 0x%04X) ", curHiddenOffset);
            printf("(Pointer at: 0x%04X)\n", HiddenObjectPointersOffset+(2*i));

           ProcessHiddenData(romBuffer+curHiddenOffset, curHiddenOffset);

        }//all maps read
        //printf("\n\n Hidden Data read");
    }
    else if (mode == DUMP_TYPETABLE)
    {
        ProcessTypeTableData(romBuffer+TypeEffectivenesOffset);
    }
    else if (mode == DUMP_TRAINERS)
    {
        ProcessTrainerData(romBuffer);
    }

    //Save output
#ifdef SWAP_PICS
    FILE * outpFile;
    outpFile = fopen ("swapped.gb", "wb");
    if (outpFile==NULL) {fputs ("Error opening out.gb",stderr); exit (1);}
    fwrite (romBuffer , sizeof(char), lSize, outpFile);
    fclose (outpFile);
#endif
  // terminate
    my_exit();

}

void my_exit()
{
    if (romBuffer) free (romBuffer);
    printf("\n\nPress enter to exit...");
    while(1)
    {
        if (getchar()) break;
    }
    exit(0);
}

uint16_t Get2bytePointerFromTable(int index, char*buffer, int tableOffset)
{
    uint16_t pointer = 0;

    memcpy(&pointer, buffer+tableOffset+(index*2), 2);

    return pointer;

}

int GetFullPointerFromTable(int index, char*buffer, int tableOffset, int bank)
{
    return ThreeByteToTwoByte(bank, Get2bytePointerFromTable(index, buffer, tableOffset));
}

void ProcessTrainerData (char*buffer)
{
    int TrainerPicAndMoneyPointers = 0x101914; //Repointed in brown to 40:5914
    int BANK_TrainerPicAndMoneyPointers = 0x40;
    int TrainerAIPointers = 0x3a55c;
    int BANK_TrainerAIPointers = 0x0e;
    int TrainerClassMoveChoiceModifications = 0x3989B;
    int BANK_TrainerClassMoveChoiceModifications = 0x0e;
    int TrainerDataPointers = 0x39d3b;
    int BANK_TrainerDataPointers = 0x0e;


    int FemaleTrainerList = 0x3434;
    int EvilTrainerList = 0x3439;

    int totalclasses = 0x2F;


    int teamcounter = 0;
    int pointer = 0;
    uint16_t u16temp = 0;

    int i = 0;
    for (i=0;i<totalclasses;i++)
    {
        printf("Trainer Class #%02d: %s (0x%02X)\n", i+1, TRAINER_CLASS[i+1], i+1);

        //Print Picture and money
        // 2 byte pointer
        // 3 bytes BCD3 money
        // pic pointer, base reward money
        // money received after battle = base money � level of last enemy mon
        pointer = TrainerPicAndMoneyPointers+(5*i);
        printf ("\t Money: %02X%02X%02X\t\t", (uint8_t)buffer[pointer+2], (uint8_t)buffer[pointer+3], (uint8_t)buffer[pointer+4]);
        printf ("Picture Offset: 0x%02X%02X", (uint8_t)buffer[pointer+1], (uint8_t)buffer[pointer]);
        printf ("\t\t(Data offset: 0x%X)\n", pointer);

        //Print AI
        // 3 byte per entry
        // one entry per trainer class
        // first byte, number of times (per Pok�mon) it can occur
        // next two bytes, pointer to AI subroutine for trainer class
        // subroutines are defined in engine/battle/trainer_ai.asm

        printf ("\t AI subroutine (Offset 0x%X): ", TrainerAIPointers+(i*3));
        memcpy(&u16temp, buffer+TrainerAIPointers+(i*3)+1, 2);
        int j = 0;
        for (j=0; j<19; j++)
        {
            if (u16temp == ItemAIPointers[j]) printf ("%s (%s)", ItemAIItems[j], ItemAINames[j] );
        }
        printf ("\tTimes it can Happen per Mon: %d\n", (uint8_t)buffer[TrainerAIPointers+(i*3)]);

        //Print AI modifications

        printf("\t AI Move choice modifications");
        int curpointer = 0;
        //advance pointer to this trainer class's data
        j=0;
        while (j<i)
        {
            if (buffer[TrainerClassMoveChoiceModifications+curpointer] == 0)
            {
                j++;
            }

            curpointer++;
        }
        printf (" (Offset 0x%X):", TrainerClassMoveChoiceModifications+curpointer);
        while (true)
        {
            printf (" %d ", buffer[TrainerClassMoveChoiceModifications+curpointer]);
            if (buffer[TrainerClassMoveChoiceModifications+curpointer+1]!=00)
            {
                printf (",");
            }
            else
            {
                printf ("\n");
                break;
            }
            curpointer++;
        }


        //Print Teams
        /*
            ; if first byte != $FF, then
            ; first byte is level (of all pokemon on this team)
            ; all the next bytes are pokemon species
            ; null-terminated
            ; if first byte == $FF, then
            ; first byte is $FF (obviously)
            ; every next two bytes are a level and species
            ; null-terminated
        */
        //printf ("\tTeams:\n");
        curpointer = GetFullPointerFromTable(i, buffer, TrainerDataPointers, BANK_TrainerDataPointers);
         printf ("\t Teams (Pointer offset 0x%X):\n", TrainerDataPointers+(i*2));
        //printf("%X", curpointer);
        //getchar();
        //int endpointer = GetFullPointerFromTable(i+1, buffer, TrainerDataPointers, BANK_TrainerDataPointers);
        int templevel = 0;
        int curteam = 1;
        bool breakflag = false;
        while (curteam <50)//dump 50 teams per trainer (max is rocket with 41) as there's no way to know when a trainer class's teams stop
        {
            if (
                        ((uint8_t)buffer[curpointer] == 0)
                    ||
                        breakflag == true
                    ||
                    (
                        ((uint8_t)buffer[curpointer] > 100)
                        &&
                        ((uint8_t)buffer[curpointer] != 0xFF)
                    )
                ) //If this happens there's no more teams...
            {
                //Break in the following situations
                //First byte of team is 0x00 (this should never happen)
                //Impossible level
                //Impossible species ID
                break;
            }
            else
            {
                printf ("\t\tTeam #%02d (Offset 0x%X): ", curteam, curpointer);
            }
            //Read team level + species
            if ((uint8_t)buffer[curpointer] == 0xFF) //Level + species
            {
                curpointer++; //advance
                while (buffer[curpointer] != 0) //Team end
                {
                    //Check if it's an impossible level
                    if ((uint8_t)buffer[curpointer] >100 || (uint8_t)buffer[curpointer] == 0)
                    {
                        breakflag = true;
                        break;
                    }
                    //Print Level
                    printf("%02d ", (uint8_t)buffer[curpointer]);
                    curpointer++;

                    //Print Species
                    printf("%0s ", SpeciesName[(uint8_t)buffer[curpointer]]);
                    curpointer++;
                }
                printf("\n");
                if (breakflag) break;
                curpointer++;
                curteam++;
                teamcounter++;
            }
            else
            {
                printf ("Team level: %02d Members: ", (uint8_t)buffer[curpointer]);
                curpointer++;
                while ((uint8_t)buffer[curpointer] != 0) //Team end
                {
                    printf("%s, ", SpeciesName[(uint8_t)buffer[curpointer]]);
                    curpointer++;
                }
                printf("\n");
                curpointer++;
                curteam++;
                teamcounter++;
            }
        }


        printf ("\n");
    }

        //Female trainer list

        //Evil trainer list

}

void ProcessHiddenData (char*buffer, int offset)
{
    /*Each hidden item has 6 bytes
    db \2 ; y coord
	db \1 ; x coord
	db \3 ; item id
	dba \4 ; object routine
    */

    uint8_t x_coord = 0;
    uint8_t y_coord = 0;
    uint8_t item_text_ID = 0;
    uint8_t predef_bank = 0;
    uint16_t predef_pointer;

    int cur_object = 0;
    int cur_buffer = 0;

    while (1)
    {
        y_coord = buffer[0+cur_buffer];
        x_coord = buffer[1+cur_buffer];
        item_text_ID = buffer[2+cur_buffer];
        predef_bank = buffer[3+cur_buffer];
        memcpy(&predef_pointer, buffer+4+cur_buffer, 2);

        if (y_coord == 0xff) break; //0xFF terminates the hidden object entries

        //Only dump hidden items
        //Identify what is what by the predefined routine the hidden object uses
        //Others are CableClubRightGameboy, OpenRedsPC, PrintBookcaseText, PrintNotebookText...
        if(predef_pointer == 0x6688)//item
        {
            printf ("\tObject %d (Addr 0x%4X):\n", cur_object, offset+(cur_object*6));
            printf ("\t\t X coord: %02d Y coord: %02d\n", x_coord, y_coord);
                printf ("\t\t Item: %s\n", ItemName[item_text_ID]);

            //No need to print the address for the predef routine
            //printf ("\t\t Bank: 0x%02X Pointer: 0x%02X\n", predef_bank, predef_pointer);
        }

        cur_buffer += 6;
        cur_object++;
    }
    printf("\tTotal hidden objects: %d\n",cur_object );
    return;
}

void ProcessEvoMoves (char*buffer)
{
    int cur_entry = 0;
    uint8_t evotype = 0;
    //Parse evolutions
    while (1)
    {
        if (buffer[cur_entry] == 0)
        {
            if (!comadump)
            {
                if (!cur_entry) printf("\n\tEvolution: NONE");
            }
            break;
        }

        evotype = buffer[cur_entry];
        if (evotype == 2)//Item evolution
        {
            if (!comadump)
            {
                printf ("\n\tEvolution: %s", EVO_TYPE[buffer[cur_entry]]);
                printf ("  Item: %s", ItemName[buffer[cur_entry+1]&0x000000FF]);
                printf ("  Level: %02d", buffer[cur_entry+2]&0x000000FF);//Always 1 for item evo
                printf ("  EvoTo: %s", SpeciesName[buffer[cur_entry+3]&0x000000FF]);
            }

            cur_entry+=4;
        }
        else if (evotype == 4)
        {
            if (!comadump)
            {
                printf ("\n\tEvolution: %s", EVO_TYPE[buffer[cur_entry]]);
                printf ("  Map: 0x%X %s", buffer[cur_entry+1]&0x000000FF, MapNames[buffer[cur_entry+1]&0x000000FF]);
                printf ("  Level: %02d", buffer[cur_entry+2]&0x000000FF);
                printf ("  EvoTo: %s", SpeciesName[buffer[cur_entry+3]&0x000000FF]);
            }
            cur_entry+=4;
        }
        else //level or trade
        {
            if (!comadump)
            {
                printf ("\n\tEvolution: %s", EVO_TYPE[buffer[cur_entry]&0x000000FF]);
                printf ("  Level: %02d", buffer[cur_entry+1]&0x000000FF);
                printf ("  EvoTo: %s", SpeciesName[buffer[cur_entry+2]&0x000000FF]);
            }
            cur_entry+=3;
        }
    }
    cur_entry++;//advance to learnset
    //Learnset
    if (!comadump)
    {
        printf("\n\tLearnset:");
    }
    while (1)
    {
        if (buffer[cur_entry] == 0)
        {
            break;
        }
        if (!comadump)
        {
            printf("\n\t\tLevel: %2d Move: %s (0x%02X)", buffer[cur_entry]&0x000000FF, MoveNames[buffer[cur_entry+1]&0x000000FF], buffer[cur_entry+1]&0x000000FF);
        }
        else
        {
            printf("%2d;%s;", buffer[cur_entry]&0x000000FF, MoveNames[buffer[cur_entry+1]&0x000000FF]);
        }
        cur_entry +=2;
    }

    return;
}

void ProcessBasestats (char*buffer)
{
    uint8_t dexnum = buffer[0];
    uint8_t hp = buffer[1];
    uint8_t atk = buffer[2];
    uint8_t def = buffer[3];
    uint8_t spd = buffer[4];
    uint8_t spc = buffer[5];
    uint8_t type1 = buffer[6];
    uint8_t type2 = buffer[7];
    uint8_t catch_rate = buffer[8];
    uint8_t base_exp = buffer[9];
    uint8_t spritedim = buffer[10];
    uint16_t picFront = 0;
    memcpy(&picFront, buffer+11, 2);
    uint16_t picBack = 0;
    memcpy(&picBack, buffer+13,  2);
    //uint8_t picfront_1 = buffer[11];//should really use uint16_t and copy the value...
    //uint8_t picfront_2 = buffer[12];
    //uint8_t picback_1 = buffer[13];//should really use uint16_t and copy the value...
    //uint8_t picback_2 = buffer[14];
    uint8_t move1 = buffer[15];
    uint8_t move2 = buffer[16];
    uint8_t move3 = buffer[17];
    uint8_t move4 = buffer[18];
    uint8_t growth = buffer[19];
    uint8_t picBank = buffer[27];

 //swap frontpic pointer with backpic pointer for easy visualization
#ifdef SWAP_PICS
        memcpy(buffer+11, &picBack, 2);
        memcpy(buffer+13, &picFront, 2);
#endif

//Species;Dexnumber;Name;HP;ATK;DEF;SPD;SPC;Type1;Type2;CatchRate;BaseExp;Move1;Move2;Move3;Move4;GrowthRate;SpriteDim;PicFront;PicBack;PicBank;
    if (comadump)
    {
        printf("%03d;%s;%02d;%02d;%02d;%02d;%02d;%s;%s;%02d;%02d;%s;%s;%s;%s;%s;", dexnum, DexOrderNames[dexnum], hp, atk, def, spd, spc, Types[type1], Types[type2], catch_rate, base_exp, MoveNames[move1], MoveNames[move2], MoveNames[move3], MoveNames[move4], GrowthRates[growth]);
    }
    else
    {
        printf("DexNum: %03d %s\n", dexnum, DexOrderNames[dexnum]);
        printf("\tHP: %02d  ATK: %02d  DEF: %02d  SPD: %02d  SPC: %02d\n", hp, atk, def, spd, spc);
        printf("\tType1: %s    Type2: %s\n", Types[type1], Types[type2]);
        printf("\tCatch Rate: %02d    Base Exp: %02d\n", catch_rate, base_exp);
        printf("\tM1: %s   M2: %s   M3: %s   M4: %s\n", MoveNames[move1], MoveNames[move2], MoveNames[move3], MoveNames[move4]);
        printf("\tGrowth: %s\n", GrowthRates[growth]);
        printf("\tFront Pic Addr: %02X:%04X% (0x%X)\tBack Pic Addr: %02X:%04X (0x%X)\n", picBank, picFront, ThreeByteToTwoByte(picBank, picFront), picBank, picBack, ThreeByteToTwoByte(picBank, picBack));
    } // COMADUMP

    uint8_t tm_byte = 0;

    int curbyte=0;
    int curTM = 0;

    if (!comadump)
    {
        printf ("\n");
    }

    //Process TM_HM bitflags (total of 55 bits 50 TM `+ 5 HM)
    for (int i=0;i<7;i++)
    {
        tm_byte = buffer[20+curbyte];
        for (int j=0; j<8;j++)
        {
            if (curTM>54) break;
            if (!comadump)
            {
                printf("\t%s: ", TM_HM[curTM]);
            }
            if (CHECK_BIT(tm_byte, j))
            {
                if (comadump)
                {
                    printf("1;");
                }
                else
                {
                    printf("1\n");
                }
            }
            else
            {
                if (comadump)
                {
                    printf("0;");
                }
                else
                {
                    printf("0\n");
                }
            }
            curTM++;
        }
        curbyte++;
    }
    return;
}

/*Move Data Structure from Red dissassembly
	db \1 ; animation (interchangeable with move id)
	db \2 ; effect
	db \3 ; power
	db \4 ; type
	db \5 percent ; accuracy
	db \6 ; pp
	assert \6 <= 40, "PP must be 40 or less"
*/
void ProcessMoveData (char*buffer)
{
    uint8_t accuracy = uint8_t(buffer[4]);
    int percent =  accuracy * 100 /0xFF;
    //Accuracy is stored as 0-FF values, need to convert to percentage
    //Parse move data
    if (!comadump)
    {
        printf("\t Anim: %s\tEffect: %s\n", Animations[(uint8_t)buffer[0]], MoveEffects[(uint8_t)buffer[1]]);
        printf("\t Power: %02d\t\tType: %s\n", (uint8_t)buffer[2], Types[(uint8_t)buffer[3]]);
        printf("\t Accuracy: %d%% (0x%02X)\tPP:%02d\n", (int)percent, (uint8_t)buffer[4], (uint8_t)buffer[5]);
    }
    else
    {
        //Separated comma values output
        //Type;power;accuracy;pp;effect;animation
        printf("%s;%02d;%d%%;%02d;%s;%s;", Types[(uint8_t)buffer[3]], (uint8_t)buffer[2], (int)percent, (uint8_t)buffer[5], MoveEffects[(uint8_t)buffer[1]], Animations[(uint8_t)buffer[0]] );
    }


    return;
}

void ProcessWildData (char*buffer)
{
#ifdef DEBUG
    printf("\t");
    for (int j=0;j<42;j++)
    {
        if ( j== 21|| j== 1|| j== 22) printf ("\n\t");
        printf("%02X ", (uint8_t)buffer[j]);
    }
    printf ("\n");
#endif // DEBUG

    char index = 0;
   if (buffer[0] != 0) //There's grass data
   {
#ifdef DEBUG
        printf("\tGrass Encounter Rate: %2X (%d)\n", (uint8_t)buffer[0], (uint8_t)buffer[0]);
#else
        printf("\tGrass Encounter Rate: %d\n", (uint8_t)buffer[0]);
#endif // DEBUG
        for (int i=0; i<10; i++)
        {
            index = ((i*2)+1);
#ifdef DEBUG
            printf("\t\tGrass Slot %d: level %02d Species: %02X, %s\n", i, (uint8_t)buffer[index], (uint8_t)buffer[index+1], SpeciesName[(uint8_t)buffer[index+1]]);
#else
            printf("\t\tGrass Slot %d: level %02d %s\n", i, (uint8_t)buffer[index], SpeciesName[(uint8_t)buffer[index+1]]);

#endif // DEBUG

        }
        //Check for water data after grass data
        if (buffer[21] != 0)
        {
#ifdef DEBUG
            printf("\tWater Encounter Rate: %2X (%d)\n", (uint8_t)buffer[21], (uint8_t)buffer[21]);
#else
            printf("\tWater Encounter Rate: %d\n", (uint8_t)buffer[21]);
#endif // DEBUG
            for (int i=0; i<10; i++)
            {
                index = ((i*2)+22);
#ifdef DEBUG
                printf("\t\tWater Slot %d: level %02d Species: %02X, %s\n", i, (uint8_t)buffer[index], (uint8_t)buffer[index+1], SpeciesName[(uint8_t)buffer[index+1]]);
#else
                printf("\t\tWater Slot %d: level %02d %s\n", i, (uint8_t)buffer[index], SpeciesName[(uint8_t)buffer[index+1]]);

#endif // DEBUG
            }
        }
        else
        {
            printf("\tNo water encounters\n");
        }
    }
    else //No grass data, check for water data
    {
        printf("\tNo grass encounters\n");
        //Check water encounters
        if (buffer[1] != 0)
        {
#ifdef DEBUG
            printf("\tWater Encounter Rate: %2X (%d)\n", (uint8_t)buffer[1], (uint8_t)buffer[1]);
#else
            printf("\tWater Encounter Rate: %d\n", (uint8_t)buffer[1]);
#endif // DEBUG
            for (int i=0; i<10; i++)
            {
                index = ((i*2)+2);
#ifdef DEBUG
                printf("\t\tWater Slot %d: level %02d Species: %02X, %s\n", i, (uint8_t)buffer[index], (uint8_t)buffer[index+1], SpeciesName[(uint8_t)buffer[index+1]]);
#else
                printf("\t\tWater Slot %d: level %02d %s\n", i, (uint8_t)buffer[index], SpeciesName[(uint8_t)buffer[index+1]]);
#endif // DEBUG
            }
        }
        else
        {
            printf("\tNo water encounters\n");
        }
    }
    return;
}

void ProcessTypeTableData (char*buffer)
{
    int curbyte = 0;

    if (!comadump)
    {
        printf("All combinations not in the table are neutral damage.\n\n");

        while (1)
        {
            printf("%s  vs  %s  *= %.2f (offset 0x%X)\n", TypesTable[buffer[curbyte]], TypesTable[buffer[curbyte+1]], ((float)buffer[curbyte+2])/10, TypeEffectivenesOffset+curbyte );
            curbyte+=3;
            if ((uint8_t)(buffer[curbyte]&0x000000FF) == 0xFF) break;
        }
    }
    else //Dump types as a type compatibility char
    {
        int totaltypes = 24;
        int current_type = 0;

        //First write the first row with all types
        printf(";");//First cell empty at column 0 and row 0
        int i = 0;
        for (i=0;i<totaltypes;i++)
        {
            printf("%s;", TypesMatchesNames[i]);
        }
        printf("\n");

        uint8_t match_type1 = 0;
        uint8_t match_type2 = 0;
        uint8_t cur_type1 = 0;
        uint8_t cur_type2 = 0;
        float effectiveness = 1;
        int j = 0;

        for (i=0;i<totaltypes;i++)
        {
            //Print the current type (column 0)
            printf("%s;", TypesMatchesNames[i]);

            //Load current type to type 1
            match_type1 = TypesMatchesIds[i];

            //Loop trough all entries of the types table and find the matchup for each type
            for (j=0;j<totaltypes;j++)
            {
                match_type2 = TypesMatchesIds[j];
                effectiveness = 1; //Reset effectiveness to neutral

                while(true)
                {
                    //Load type1
                    cur_type1 = (uint8_t)(buffer[curbyte]&0x000000FF);
                    //Load type2
                    cur_type2 = (uint8_t)(buffer[curbyte+1]&0x000000FF);

                    //Check if it is what we are looking for
                    if (match_type1 == cur_type1 && match_type2 == cur_type2)
                    {
                        //Match, dump effectivenes
                        effectiveness = ((float)buffer[curbyte+2])/10; //x2 is stored as 0x20, x0.5 stored as 0x05 etc
                        //printf("%.2f;",effectiveness);
                        //curbyte = 0; //Back to table start
                        //break; //The game keeps looping until the end of the table, on repeated matchups only the last one counts

                    }

                    //If we reach the end of the data...
                    if ((uint8_t)(buffer[curbyte]&0x000000FF) == 0xFF)
                    {
                        //At this point:
                        // If a match was found, effectiveness was modified
                        // If not, it will have kept its 1.0 default value we set before each loop
                        printf("%.2f;",effectiveness);
                        curbyte = 0; //Back to table start
                        break;
                    }
                    curbyte+=3;//Next entry
                }

            }
            printf("\n");//New row for next type
        }
    }
}

// Byte swap signed short
uint16_t swap_uint16( uint16_t val )
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

int ThreeByteToTwoByte (int bank, uint16_t off)
{
    return ((bank * 0x4000) + (off - 0x4000));
}

int ThreeByteToTwoByteBswap (int bank, uint16_t off)
{
    return ((bank * 0x4000) + (swap_uint16(off) - 0x4000));
}

int getBank (int off)
{
    return off / 0x4000;
}

int TwoByteOffset (int off)
{
    return (off % 0x4000) + 0x4000;
}

