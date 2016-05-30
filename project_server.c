#ifdef _WIN32_WCE
#undef UNICODE
#undef _UNICODE
#define main my_main
#endif


#include "ctdbsdk.h" /* c-tree headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include <rtai_lxrt.h>
#include <sys/ioctl.h>


#define LOCK_SUPPORT /*lock*/

#ifndef ctCLIENT
#ifdef NOTFORCE
#undef LOCK_SUPPORT
#endif
#endif

#define END_OF_FILE INOT_ERR  /* INOT_ERR is ctree's 101 error. See cterrc.h */


/* Global declarations */

CTHANDLE hSession;

CTHANDLE hDatabase;

CTHANDLE hTableInfo; //user info: UserNo, name, gender, age
CTHANDLE hTableConnRecd; //record of connection:UserNo, monitor date, start time, end time
CTHANDLE hTableSignalRecd; //record of signal: UserNo, time, channel1, channel2, channel3, channel4

CTHANDLE hRecordInfo;
CTHANDLE hRecordConnRecd;
CTHANDLE hRecordSignalRecd;




/* Function declarations */

#ifdef PROTOTYPE
VOID Initialize(VOID), Define(VOID), Manage(VOID), Done(VOID);

VOID Create_Info_Table(VOID), Create_ConnRecd_Table(VOID);
VOID Create_SignalRecd_Table(VOID);

VOID Add_Info_Records(VOID), Add_ConnRecd_Records(char*,int,int);
VOID Add_SignalRecd_Records(SignalRecd_DATA);

VOID Display_Info(VOID), Display_ConnRecd();
VOID Display_SignalRecd();

VOID Delete_Records(CTHANDLE hRecord);
VOID Check_Table_Mode(CTHANDLE hTable);
VOID update_ConnRecd(CTSTRING,int,int,int);

VOID Handle_Error(CTSTRING);
#else
VOID Initialize(), Define(), Manage(), Done();

VOID Create_Info_Table(), Create_ConnRecd_Table();
VOID Create_SignalRecd_Table();

VOID Add_Info_Records(), Add_ConnRecd_Records();
VOID Add_SignalRecd_Records();

VOID Display_Info(), Display_ConnRecd();
VOID Display_SignalRecd();

VOID Delete_Records();
VOID Check_Table_Mode();
VOID update_ConnRecd()

VOID Handle_Error();
#endif

//server part

#define MSG_SIZE 68			// message size
typedef struct { //signal record data sturcture
	COUNT usrno,sequence;
    CTSTRING time;
    CTSTRING channel1, channel2, channel3, channel4;
} SignalRecd_DATA;
void dostuff(int); 			// function prototype

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

 //set up server and wait for connection
void start_server(NINT argc,pTEXT argv[])
{
	int sockfd, newsockfd, portno, pid, j = 0;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;


	if (argc < 2)
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0); // Creates socket. Connection based.
	if (sockfd < 0)
		error("ERROR opening socket");

	// fill in fields
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);	// get port number from input
	serv_addr.sin_family = AF_INET;		 // symbol constant for Internet domain
	serv_addr.sin_addr.s_addr = INADDR_ANY; // IP address of the machine on which
										 // the server is running
	serv_addr.sin_port = htons(portno);	 // port number

	// binds the socket to the address of the host and the port number
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd, 5);			// listen for connections
	clilen = sizeof(cli_addr);	// size of structure


	while (1)
	{
		// blocks until a client connects to the server
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0)
			error("ERROR on accept");

		j++;		// counter for the connections that are established.
		pid = fork();
		if (pid < 0)
			error("ERROR on fork");

		if (pid == 0)	// child process
		{
			printf("Connection #%d created\n",j);
			close(sockfd);			// close socket
			dostuff(newsockfd);	// call function that handles communication
			printf("end %d\n",j);
			exit(0);

		}
		else			// parent
		{
			close(newsockfd);
			signal(SIGCHLD,SIG_IGN);	// to avoid zombie problem
		}
	} 	// end of while

	close(sockfd);

}




/*
 * main()
 *
 * The main() function implements the concept of "init, define, manage
 * and you're done..."
 */

#ifdef PROTOTYPE
NINT main (NINT argc, pTEXT argv[])
#else
NINT main (argc, argv)
NINT argc;
TEXT argv[];
#endif
{


   Initialize(); //link to faircom

   Define();  //create or open tables

   //socket
   start_server(argc,argv);

	//use it and choose the function in manage to delete or display the table
 	//Manage();  

   Done();

   printf("\nPress <ENTER> key to exit . . .\n");
#ifndef ctPortWINCE
   getchar();
#endif

   return(0);
}


/*
 * Initialize()
 *
 * Perform the minimum requirement of logging onto the c-tree Server
 */

#ifdef PROTOTYPE
VOID Initialize(VOID)
#else
VOID Initialize()
#endif
{
   CTDBRET  retval;

   printf("INIT\n");

   if ((retval = ctdbStartDatabaseEngine())) 				/* This function is required when you are using the Server DLL model to start the underlying Server. */
	Handle_Error("Initialize(): ctdbStartDatabaseEngine()"); 	/* It does nothing in all other c-tree models */

   /* allocate session handle */
   if ((hSession = ctdbAllocSession(CTSESSION_CTREE)) == NULL)
      Handle_Error("Initialize(): ctdbAllocSession()");

   hDatabase = hSession;

   /* connect to server */
   printf("\tLogon to server...\n");
   if (ctdbLogon(hSession, "FAIRCOMS", "ADMIN", "ADMIN"))
      Handle_Error("Initialize(): ctdbLogon()");

}


/*
 * Define()
 *
 * Open the table, if it exists. Otherwise create and open the table
 */

#ifdef PROTOTYPE
VOID Define(VOID)
#else
VOID Define()
#endif
{
	//3 tables
	Create_Info_Table();
	Create_ConnRecd_Table();
	Create_SignalRecd_Table();

	if ((hRecordInfo = ctdbAllocRecord(hTableInfo)) == NULL)
		Handle_Error("Add_Info_Records(): ctdbAllocRecord()");
	if ((hRecordConnRecd = ctdbAllocRecord(hTableConnRecd)) == NULL)
		Handle_Error("Add_ConnRecd_Records(): ctdbAllocRecord()");
	if ((hRecordSignalRecd = ctdbAllocRecord(hTableSignalRecd)) == NULL)
		Handle_Error("Add_SignalRecd_Records(): ctdbAllocRecord()");
}


/*
 * Manage()
 *
 * This function performs simple record functions of add, delete and gets
 */

#ifdef PROTOTYPE
VOID Manage(VOID)
#else
VOID Manage()
#endif
{

	  //Delete_Records(hRecordInfo);
	 //Delete_Records(hRecordConnRecd);
	//Delete_Records(hRecordSignalRecd);

    //Add_Info_Records();


      //Display_Info();
      //Display_ConnRecd();
     //Display_SignalRecd();

}


/*
 * Done()
 *
 * This function handles the housekeeping of closing tables and
 * freeing of associated memory
 */

#ifdef PROTOTYPE
VOID Done(VOID)
#else
VOID Done()
#endif
{
   printf("DONE\n");


   /* close table */
   printf("\tClose table...\n");
   if (ctdbCloseTable(hTableInfo))
      Handle_Error("Done(): ctdbCloseTable()");
   if (ctdbCloseTable(hTableConnRecd))
      Handle_Error("Done(): ctdbCloseTable()");
   if (ctdbCloseTable(hTableSignalRecd))
      Handle_Error("Done(): ctdbCloseTable()");

   /* logout */
   printf("\tLogout...\n");
   if (ctdbLogout(hSession))
      Handle_Error("Done(): ctdbLogout()");

   /* free handles */
   ctdbFreeRecord(hRecordInfo);
   ctdbFreeRecord(hRecordConnRecd);
   ctdbFreeRecord(hRecordSignalRecd);

   ctdbFreeTable(hTableInfo);
   ctdbFreeTable(hTableConnRecd);
   ctdbFreeTable(hTableSignalRecd);

   ctdbFreeSession(hSession);

   /* If you are linked to the Server DLL, then we should stop our Server at the end of the program.   */
   /* It does nothing in all other c-tree models */
   ctdbStopDatabaseEngine();
}

/*
 * Create_Create_Info_Table_Table()
 *
 * Open table Info, if it exists. Otherwise create it
 * along with its indices and open it
 */

#ifdef PROTOTYPE
VOID Create_Info_Table(VOID)
#else
VOID Create_Info_Table()
#endif
{
   CTHANDLE pField1, pField2, pField3, pField4;
   CTHANDLE pIndex;
   CTHANDLE pIseg;

   /* define table CustomerMaster */
   printf("\table info\n");

   /* allocate a table handle */
   if ((hTableInfo = ctdbAllocTable(hDatabase)) == NULL)
      Handle_Error("Create_info_Table(): ctdbAllocTable()");

   /* open table */
   if (ctdbOpenTable(hTableInfo, "UserInfo", CTOPEN_NORMAL))
   {
      /* define table fields */
      pField1 = ctdbAddField(hTableInfo, "In_userno", CT_INT2, 2);
      pField2 = ctdbAddField(hTableInfo, "In_name", CT_STRING, 47);
      pField3 = ctdbAddField(hTableInfo, "In_gender", CT_FSTRING, 1);
      pField4 = ctdbAddField(hTableInfo, "In_age", CT_FSTRING, 3);


      if (!pField1 || !pField2 || !pField3 || !pField4 )
         Handle_Error("Create_CustomerMaster_Table(): ctdbAddField()");

      /* define index . usrno*/
      pIndex = ctdbAddIndex(hTableInfo, "in_userno_idx", CTINDEX_PADDING, NO, NO);
      pIseg = ctdbAddSegment(pIndex, pField1, CTSEG_SCHSEG);
      if (!pIndex || !pIseg)
         Handle_Error("Create_CustomerMaster_Table(): ctdbAddIndex()|ctdbAddSegment()");

      /* create table */
      if (ctdbCreateTable(hTableInfo, "UserInfo", CTCREATE_NORMAL))
         Handle_Error("Create_CustomerMaster_Table(): ctdbCreateTable()");

      /* open table */
      if (ctdbOpenTable(hTableInfo, "UserInfo", CTOPEN_NORMAL))
         Handle_Error("Create_CustomerMaster_Table(): ctdbOpenTable()");
   }
   else
   {
      Check_Table_Mode(hTableInfo);

   }
}

/*
 * Create_Create_ConnRecd_Table_Table()
 *
 * Open table ConnRecd, if it exists. Otherwise create it
 * along with its indices and open it
 */

#ifdef PROTOTYPE
VOID Create_ConnRecd_Table(VOID)
#else
VOID Create_ConnRecd_Table()
#endif
{
   CTHANDLE pField1, pField2, pField3, pField4,pField5;
   CTHANDLE pIndex1,pIndex2;
   CTHANDLE pIseg1,pIseg2,pIseg3;

   /* define table CustomerMaster */
   printf("\table ConnRecd\n");

   /* allocate a table handle */
   if ((hTableConnRecd = ctdbAllocTable(hDatabase)) == NULL)
      Handle_Error("Create_ConnRecd_Table(): ctdbAllocTable()");

   /* open table */
   if (ctdbOpenTable(hTableConnRecd, "ConnRecd", CTOPEN_NORMAL))
   {
      /* define table fields */
      pField1 = ctdbAddField(hTableConnRecd, "Cr_userno", CT_INT2, 2);
      pField2 = ctdbAddField(hTableConnRecd, "Cr_sequence", CT_INT2, 2);
      pField3 = ctdbAddField(hTableConnRecd, "Cr_date", CT_DATE, 4);
      pField4 = ctdbAddField(hTableConnRecd, "Cr_starttime", CT_FSTRING, 13);
      pField5 = ctdbAddField(hTableConnRecd, "Cr_endtime", CT_FSTRING, 13);


      if (!pField1 || !pField2 || !pField3 || !pField4 )
         Handle_Error("Create_ConnRecd_Table(): ctdbAddField()");

      /* define index usrno can be dupliex*/
      pIndex1 = ctdbAddIndex(hTableConnRecd, "cr_userno_idx", CTINDEX_PADDING, YES, NO);
      pIseg1 = ctdbAddSegment(pIndex1, pField1, CTSEG_SCHSEG);
			//usrno and sequence, can't be dupliex
      pIndex2 = ctdbAddIndex(hTableConnRecd, "cr_usersequence_idx", CTINDEX_PADDING, NO, NO);
      pIseg2 = ctdbAddSegment(pIndex2, pField1, CTSEG_SCHSEG);
      pIseg3 = ctdbAddSegment(pIndex2, pField2, CTSEG_SCHSEG);

      if (!pIndex1 || !pIndex2 || !pIseg1 || !pIseg2 || !pIseg3)
         Handle_Error("Create_ConnRecd_Table(): ctdbAddIndex()|ctdbAddSegment()");

      /* create table */
      if (ctdbCreateTable(hTableConnRecd, "ConnRecd", CTCREATE_NORMAL))
         Handle_Error("Create_ConnRecd_Table(): ctdbCreateTable()");

      /* open table */
      if (ctdbOpenTable(hTableConnRecd, "ConnRecd", CTOPEN_NORMAL))
         Handle_Error("Create_ConnRecd_Table(): ctdbOpenTable()");
   }
   else
   {
      Check_Table_Mode(hTableConnRecd);

   }
}


/*
 * Create_Create_SignalRecd_Table_Table()
 *
 * Open table SignalRecd, if it exists. Otherwise create it
 * along with its indices and open it
 */

#ifdef PROTOTYPE
VOID Create_SignalRecd_Table(VOID)
#else
VOID Create_SignalRecd_Table()
#endif
{
   CTHANDLE pField1, pField2, pField3, pField4;
   CTHANDLE pField5, pField6, pField7;
   CTHANDLE pIndex1,pIndex2;
   CTHANDLE pIseg1,pIseg2,pIseg3,pIseg4,pIseg5;

   /* define table CustomerMaster */
   printf("\table ConnRecd\n");

   /* allocate a table handle */
   if ((hTableSignalRecd = ctdbAllocTable(hDatabase)) == NULL)
      Handle_Error("Create_SignalRecd_Table(): ctdbAllocTable()");

   /* open table */
   if (ctdbOpenTable(hTableSignalRecd, "SignalRecd", CTOPEN_NORMAL))
   {

	   /* define table fields */
      pField1 = ctdbAddField(hTableSignalRecd, "Sr_userno", CT_INT2, 2);
      pField2 = ctdbAddField(hTableSignalRecd, "Sr_sequence", CT_INT2, 2);
      pField3 = ctdbAddField(hTableSignalRecd, "Sr_time", CT_FSTRING, 13);
      pField4 = ctdbAddField(hTableSignalRecd, "Sr_channel1", CT_STRING, 11);
      pField5 = ctdbAddField(hTableSignalRecd, "Sr_channel2", CT_STRING, 11);
      pField6 = ctdbAddField(hTableSignalRecd, "Sr_channel3", CT_STRING, 11);
      pField7 = ctdbAddField(hTableSignalRecd, "Sr_channel4", CT_STRING, 11);


      if (!pField1 || !pField2 || !pField3 || !pField4 || !pField5|| !pField6|| !pField7)
         Handle_Error("Create_SignalRecd_Table(): ctdbAddField()");

      /* define index  usrno and sequence can be dupliex*/
        pIndex1 = ctdbAddIndex(hTableSignalRecd, "sr_usersequence_idx", CTINDEX_PADDING, YES, NO);
        pIseg1 = ctdbAddSegment(pIndex1, pField1, CTSEG_SCHSEG);
        pIseg2 = ctdbAddSegment(pIndex1, pField2, CTSEG_SCHSEG);
			//usrno sequence and time can't be dupliex
        pIndex2 = ctdbAddIndex(hTableSignalRecd, "sr_seqtime_idx", CTINDEX_PADDING, NO, NO);
       	pIseg3 = ctdbAddSegment(pIndex2, pField1, CTSEG_DESCENDING);//time is in descending mode
       	pIseg4 = ctdbAddSegment(pIndex2, pField2, CTSEG_DESCENDING);//so the display time will be in order
       	pIseg5 = ctdbAddSegment(pIndex2, pField3, CTSEG_DESCENDING);

      if (!pIndex1 || !pIseg1 || !pIseg2 ||!pIndex2|| !pIseg3|| !pIseg4|| !pIseg5)
         Handle_Error("Create_ConnRecd_Table(): ctdbAddIndex()|ctdbAddSegment()");

      /* create table */
      if (ctdbCreateTable(hTableSignalRecd, "SignalRecd", CTCREATE_NORMAL))
         Handle_Error("Create_SignalRecd_Table(): ctdbCreateTable()");

      /* open table */
      if (ctdbOpenTable(hTableSignalRecd, "SignalRecd", CTOPEN_NORMAL))
         Handle_Error("Create_SignalRecd_Table(): ctdbOpenTable()");
   }
   else
   {
	   Check_Table_Mode(hTableSignalRecd);


   }
}


/*
 * Check_Table_Mode()
 *
 * Check if existing table has transaction processing flag enabled.
 * If a table is under transaction processing control, modify the
 * table mode to disable transaction processing
 */

#ifdef PROTOTYPE
VOID Check_Table_Mode(CTHANDLE hTable)
#else
VOID Check_Table_Mode(hTable)
CTHANDLE hTable;
#endif
{
   CTCREATE_MODE mode;

   /* get table create mode */
   mode = ctdbGetTableCreateMode(hTable);

   /* check if table is under transaction processing control */
   if ((mode & CTCREATE_TRNLOG))
   {
      /* change file mode to disable transaction processing */
      mode ^= CTCREATE_TRNLOG;
      if (ctdbUpdateCreateMode(hTable, mode) != CTDBRET_OK)
         Handle_Error("Check_Table_Mode(); ctdbUpdateCreateMode");
   }
}


/*
 * Delete_Records()
 *
 * This function deletes all the records in the table
 */

#ifdef PROTOTYPE
VOID Delete_Records(CTHANDLE hRecord)
#else
VOID Delete_Records(hRecord)
CTHANDLE hRecord;
#endif
{
   CTDBRET  retval;
   CTBOOL   empty;

   printf("\tDelete records...\n");

   /* enable session-wide lock flag */
   if (ctdbLock(hSession, CTLOCK_WRITE_BLOCK))
   	Handle_Error("Delete_Records(): ctdbLock()");

   empty = NO;
   retval = ctdbFirstRecord(hRecord);
   if (retval != CTDBRET_OK)
   {
      if (retval == END_OF_FILE)
         empty = YES;
      else
         Handle_Error("Delete_Records(): ctdbFirstRecord()");
   }

   while (empty == NO) /* while table is not empty */
   {
      /* delete record */
      if (ctdbDeleteRecord(hRecord))
         Handle_Error("Delete_Records(): ctdbDeleteRecord()");

      /* read next record */
      retval = ctdbNextRecord(hRecord);
      if (retval != CTDBRET_OK)
      {
         if (retval == END_OF_FILE)
            empty = YES;
         else
            Handle_Error("Delete_Records(): ctdbNextRecord()");
      }
   }
   if (ctdbUnlock(hSession))
   	Handle_Error("Delete_Records(): ctdbLock()");
}


/*
 * Add_Info_Records()
 *
 * This function adds Info_Records to a table in the database
 */

typedef struct {
	COUNT usrno;
   CTSTRING name,gender,age;
} INFO_DATA;

INFO_DATA data1[]=
   {{1, "aaa", "F", "22"},{2, "bb", "F", "69"},{3, "cc", "M", "919"},{4, "dd", "F", "22"},
	{5, "ee", "M", "30"},{6, "ff", "M", "22"},{7, "gg", "F", "22"},{8, "hh", "F", "27"}
   }; //pre-exist records

#ifdef PROTOTYPE
VOID Add_Info_Records(VOID)
#else
VOID Add_Info_Records()
#endif
{
   CTDBRET  retval;
   CTSIGNED i;
   CTSIGNED nRecords = sizeof(data1) / sizeof(INFO_DATA);

   printf("\tAdd records...\n");
   /* add data to table */


   //Delete_Records(hRecordInfo);

   for (i=0;i<nRecords;i++)
   {
	   /* clear record buffer */
	   ctdbClearRecord(hRecordInfo);

	   retval = 0;
	   /* populate record buffer with data */
	   retval |= ctdbSetFieldAsSigned(hRecordInfo, 0, data1[i].usrno);
	   retval |= ctdbSetFieldAsString(hRecordInfo, 1, data1[i].name);
	   retval |= ctdbSetFieldAsString(hRecordInfo, 2, data1[i].gender);
	   retval |= ctdbSetFieldAsString(hRecordInfo, 3, data1[i].age);

	   if (retval)
		   Handle_Error("Add_hRecordInfo(): ctdbSetFieldAsString()");



	   /* add record */
	   if (ctdbWriteRecord(hRecordInfo))
		   printf("Add_hRecordInfo(): ctdbWriteRecord()");
         //Handle_Error("Add_hRecordInfo(): ctdbWriteRecord()");
   }
}

/*
 * Add_ConnRecd_Records()
 *
 * This function adds ConnRecd records to a table in the database
 */

typedef struct {
	COUNT usrno,sequence;
   CTSTRING date,starttime,endtime;
} ConnRecd_DATA;


#ifdef PROTOTYPE
VOID Add_ConnRecd_Records(char* date_string ,int usrno ,int sequence)
#else
VOID Add_ConnRecd_Records()
#endif
{
   CTDBRET  retval;
   CTDATE   date;

   ConnRecd_DATA data2; 
   data2.usrno=usrno;
   data2.sequence=sequence;
   data2.starttime=""; //at first no start and end time
   data2.endtime="";


   printf("\tAdd records...\n");
   /* add data to table */

   /* clear record buffer */
   ctdbClearRecord(hRecordConnRecd);

   retval = 0;
   retval |= ctdbStringToDate(date_string, CTDATE_MDCY ,&date); //convert to data type
   /* populate record buffer with data */
   retval |= ctdbSetFieldAsSigned(hRecordConnRecd, 0, data2.usrno);
   retval |= ctdbSetFieldAsSigned(hRecordConnRecd, 1,data2.sequence);
   retval |= ctdbSetFieldAsDate(hRecordConnRecd, 2, date);
   retval |= ctdbSetFieldAsString(hRecordConnRecd, 3, data2.starttime);
   retval |= ctdbSetFieldAsString(hRecordConnRecd, 4, data2.endtime);


   if (retval)
      Handle_Error("Add_hRecordConnRecd(): ctdbSetFieldAsString()");


   ctdbLock(hTableConnRecd, CTLOCK_WRITE_BLOCK); //tablelock
   if (ctdbWriteRecord(hRecordConnRecd))
	   printf("Add_hRecordConnRecd(): ctdbWriteRecord()");

   ctdbUnlockTable(hTableConnRecd);
}


/*
 * Add_SignalRecd_Records()
 *
 * This function adds signal records to a table in the database
 */

#ifdef PROTOTYPE
VOID Add_SignalRecd_Records(SignalRecd_DATA data3)
#else
VOID Add_SignalRecd_Records()
#endif
{
   CTDBRET  retval;

    printf("%d %d %s %s %s %s %s\n",data3.usrno,data3.sequence,data3.time,data3.channel1,
   		  data3.channel2,data3.channel3,data3.channel4);


   /* add data to table */

   /* clear record buffer */
    ctdbClearRecord(hRecordSignalRecd);

   retval = 0;
   /* populate record buffer with data */
   retval |= ctdbSetFieldAsSigned(hRecordSignalRecd, 0, data3.usrno);
   retval |= ctdbSetFieldAsSigned(hRecordSignalRecd, 1, data3.sequence);
   retval |= ctdbSetFieldAsString(hRecordSignalRecd, 2, data3.time);
   retval |= ctdbSetFieldAsString(hRecordSignalRecd, 3, data3.channel1);
   retval |= ctdbSetFieldAsString(hRecordSignalRecd, 4, data3.channel2);
   retval |= ctdbSetFieldAsString(hRecordSignalRecd, 5, data3.channel3);
   retval |= ctdbSetFieldAsString(hRecordSignalRecd, 6, data3.channel4);

  if (retval)
      Handle_Error("Add_hRecordSignalRecd(): ctdbSetFieldAsString()");

  //lock and write
  ctdbLock(hTableSignalRecd, CTLOCK_WRITE_BLOCK);

  ctdbWriteRecord(hRecordSignalRecd);

  ctdbUnlockTable(hTableSignalRecd);


}



/*
 * Display_Info()
 *
 * This function displays the contents of a table. ctdbFirstRecord() and
 * ctdbNextRecord() fetch the record. Then each field is parsed and displayed
 */

#ifdef PROTOTYPE
VOID Display_Info(VOID)
#else
VOID Display_Info()
#endif
{
   CTDBRET  retval;
   CTSIGNED    usrno;
   TEXT     name[47+1];
   TEXT     gender[1+1];
   TEXT     age[3+1];

   printf("\tDisplay records...");

   /* read first record */
   retval = ctdbFirstRecord(hRecordInfo);
   if (retval != CTDBRET_OK)
      Handle_Error("Display_hRecordInfo(): ctdbFirstRecord()");

   while (retval != END_OF_FILE)
   {
      retval = 0;
      retval |= ctdbGetFieldAsSigned(hRecordInfo, 0, &usrno);
      retval |= ctdbGetFieldAsString(hRecordInfo, 1, name, sizeof(name));
      retval |= ctdbGetFieldAsString(hRecordInfo, 2, gender, sizeof(gender));
      retval |= ctdbGetFieldAsString(hRecordInfo, 3, age, sizeof(age));


      if (retval)
         Handle_Error("Display_hRecordInfo(): ctdbGetFieldAsString()");

      printf("\n\t\t%lu %s %s %s\n",usrno, name,gender,age);

      /* read next record */
      retval = ctdbNextRecord(hRecordInfo);
      if (retval == END_OF_FILE)
         break;   /* reached end of file */

      if (retval != CTDBRET_OK)
         Handle_Error("Display_hRecordInfo(): ctdbNextRecord()");
   }
}


/*
 * Display_ConnRecd()
 *
 * This function displays the contents of a table. ctdbFirstRecord() and
 * ctdbNextRecord() fetch the record. Then each field is parsed and displayed
 */

#ifdef PROTOTYPE
VOID Display_ConnRecd()
#else
VOID Display_ConnRecd()
#endif
{
   CTDBRET  retval;
   CTSIGNED    usrno;
   CTSIGNED    sequence;
   CTDATE    date;
   TEXT    date_s[10+1];
   TEXT     starttime[13+1];
   TEXT     endtime[13+1];



   printf("\tDisplay records...");

   /* read first record */
   retval = ctdbFirstRecord(hRecordConnRecd);
   if (retval != CTDBRET_OK)
      Handle_Error("Display_hRecordConnRecd(): ctdbFirstRecord()");

   while (retval != END_OF_FILE)
   {
      retval = 0;
      retval |= ctdbGetFieldAsSigned(hRecordConnRecd, 0, &usrno);
      retval |= ctdbGetFieldAsSigned(hRecordConnRecd, 1, &sequence);
      retval |= ctdbGetFieldAsDate(hRecordConnRecd, 2, &date);
      retval |= ctdbGetFieldAsString(hRecordConnRecd, 3, starttime, sizeof(starttime));
      retval |= ctdbGetFieldAsString(hRecordConnRecd, 4, endtime, sizeof(endtime));

      /* convert date to vision type*/
      retval |= ctdbDateToString(date, CTDATE_MDCY, date_s,sizeof(date_s));

      if (retval)
         Handle_Error("Display_hRecordConnRecd(): ctdbGetFieldAsString()");


      printf("\n\t\t%lu %lu %s %s %s\n",usrno,sequence,date_s,starttime,endtime);

      /* read next record */
      retval = ctdbNextRecord(hRecordConnRecd);
      if (retval == END_OF_FILE)
         break;   /* reached end of file */

      if (retval != CTDBRET_OK)
         Handle_Error("Display_hRecordConnRecd(): ctdbNextRecord()");
   }
}

/*
 * Display_SignalRecd();()
 *
 * This function displays the contents of a table. ctdbFirstRecord() and
 * ctdbNextRecord() fetch the record. Then each field is parsed and displayed
 */

#ifdef PROTOTYPE
VOID Display_SignalRecd()
#else
VOID Display_SignalRecd()
#endif
{
   CTDBRET  retval=CTDBRET_OK;
   CTSIGNED    usrno;
   CTSIGNED    sequence;
   TEXT     time[13+1];
   TEXT     channel1[11+1];
   TEXT     channel2[11+1];
   TEXT     channel3[11+1];
   TEXT     channel4[11+1];

   printf("\tDisplay records...");

   /* read first record */
   retval = ctdbFirstRecord(hRecordSignalRecd);
   if (retval != CTDBRET_OK)
      Handle_Error("Display_hRecordSignalRecd(): ctdbFirstRecord()");

  while (retval != END_OF_FILE)
  {
      retval = 0;
      retval |= ctdbGetFieldAsSigned(hRecordSignalRecd, 0, &usrno);
      retval |= ctdbGetFieldAsSigned(hRecordSignalRecd, 1, &sequence);
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 2, time, sizeof(time));
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 3, channel1,sizeof(channel1));
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 4, channel2,sizeof(channel2));
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 5, channel3,sizeof(channel3));
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 6, channel4,sizeof(channel4));


      if (retval)
         Handle_Error("Display_hRecordSignalRecd(): ctdbGetFieldAsString()");

      printf("\n\t\t%lu %lu %s %s %s %s %s\n",usrno,sequence,time,channel1,channel2,channel3,channel4);

      /* read next record */
     retval = ctdbNextRecord(hRecordSignalRecd);
      if (retval == END_OF_FILE)
        break;   /* reached end of file */

      if (retval != CTDBRET_OK)
    	  Handle_Error("Display_hRecordSignalRecd(): ctdbNextRecord()");
  }
}


/*
 * Handle_Error()
 *
 * This function is a common bailout routine. It displays an error message
 * allowing the user to acknowledge before terminating the application
 */

#ifdef PROTOTYPE
VOID Handle_Error(CTSTRING errmsg)
#else
VOID Handle_Error(errmsg)
CTSTRING errmsg;
#endif
{
   printf("\nERROR: [%d] - %s \n", ctdbGetError(hSession), errmsg);
   printf("*** Execution aborted *** \nPress <ENTER> key to exit...");

   ctdbLogout(hSession);

   ctdbFreeRecord(hRecordInfo);
   ctdbFreeRecord(hRecordConnRecd);
   ctdbFreeRecord(hRecordSignalRecd);

   ctdbFreeTable(hTableInfo);
   ctdbFreeTable(hTableConnRecd);
   ctdbFreeTable(hTableSignalRecd);

   ctdbFreeSession(hSession);


   getchar();

   exit(1);
}

/*updat the connect start time and end time. at the begining and end of the transfer*/
void update_ConnRecd(CTSTRING time,int fieldNum,int usrno,int sequence)
{
	printf("%d,%d,%s\n",usrno,sequence,time);
	if(ctdbClearRecord(hRecordConnRecd))//clear record handle
		Handle_Error("update");
	if(ctdbSetDefaultIndex(hRecordConnRecd,1)) //set index
		Handle_Error("check_usrno():ctdbSetDefaultIndex");
	if(ctdbSetFieldAsSigned(hRecordConnRecd,0,usrno)) 
		Handle_Error("check_usrno():ctdbSetFieldAsSigned");
	if(ctdbSetFieldAsSigned(hRecordConnRecd,1,sequence))
		Handle_Error("check_usrno():ctdbSetFieldAsSigned");
	if(ctdbFindRecord(hRecordConnRecd,CTFIND_EQ)!=CTDBRET_OK) //quire
		printf("record not found\n");
	else //find record
	{
		 ctdbLockRecord(hRecordConnRecd, CTLOCK_WRITE_BLOCK);

		  if(ctdbSetFieldAsString(hRecordConnRecd,fieldNum,time))
		      Handle_Error("update:set");
		  if(ctdbWriteRecord(hRecordConnRecd)!=CTDBRET_OK)//update
		      Handle_Error("update:write");

		   ctdbUnlockRecord(hRecordConnRecd);

	}
}

/*check if usrno exist in info table. call at the begining of the connection*/
int check_usrno(int usrno)
{
	int usr_check=0; //check if usrno exist. 0: no, 1: yes
	if(ctdbClearRecord(hRecordInfo))
		Handle_Error("check_usrno():ctdbClearRecord");
	if(ctdbSetFieldAsSigned(hRecordInfo,0,usrno))
		Handle_Error("check_usrno():ctdbSetFieldAsSigned");
	if(ctdbFindRecord(hRecordInfo,CTFIND_EQ)!=CTDBRET_OK)//find
		printf("record not found\n");
	else
		usr_check=1;  //found record

	return usr_check;
}

/*check which sequence is this time*/
int check_sequence(int usrno)
{
	CTDBRET retval;
	CTSIGNED t_sequence; //sequence of usrno in table connect record
	int sequence=0; //
	CTBOOL old_usr; //there are records related to this user
	if(ctdbClearRecord(hRecordConnRecd))
		Handle_Error("check_sequence():ctdbClearRecord");
	if(ctdbSetFieldAsSigned(hRecordConnRecd,0,usrno))
		Handle_Error("check_sequence():ctdbSetFieldAsSigned");
	if(ctdbRecordSetOn(hRecordConnRecd,2)) //create set. because one user can have more 
																					//than one connection record
		Handle_Error("check_sequence():ctdbRecordSetOn");
	else
	{
		if(ctdbFirstRecord(hRecordConnRecd))
		{
			sequence=0;  //found record;
		}
		else
		{
			old_usr=YES;
			while(old_usr)
			{
				sequence++; //count sequence
				if(ctdbGetFieldAsSigned(hRecordConnRecd,1,&t_sequence))
					Handle_Error("check_sequence():ctdbGetFieldAsSigned");
				//read next record
				retval=ctdbNextRecord(hRecordConnRecd);
				if(retval!=CTDBRET_OK)
				{
					if(retval==END_OF_FILE)
						old_usr=NO;
					else
						Handle_Error("check_sequence():ctdbNextRecord");
				}
			}
			ctdbRecordSetOff(hRecordConnRecd);
		}

	}
	return sequence;
}

/*send signal to monitoring program*/
VOID Send_SignalRecd(CTHANDLE hRecordSignalRecd,int sendsock,int send_num,int s_usrno,int s_sequence)
{
   CTDBRET  retval=CTDBRET_OK;
   CTSIGNED    usrno;
   CTSIGNED    sequence;
   TEXT     time[13+1];
   TEXT     channel1[11+1];
   TEXT     channel2[11+1];
   TEXT     channel3[11+1];
   TEXT     channel4[11+1];
   char buffer[MSG_SIZE]; //data
   char buffer2[MSG_SIZE]; //receive confirm message
   char buffer3[MSG_SIZE]; //send confirm message
   int n,i=0;


   memset(buffer3,0,MSG_SIZE);
   strcpy(buffer3,"y"); //confirm message

   while (i<send_num) //if not reach the data within the time range
   {
	  ctdbLock(hTableSignalRecd, CTLOCK_SUSPEND); //suspend lock
      retval = 0;
      retval |= ctdbGetFieldAsSigned(hRecordSignalRecd, 0, &usrno);
      retval |= ctdbGetFieldAsSigned(hRecordSignalRecd, 1, &sequence);
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 2, time, sizeof(time));
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 3, channel1,sizeof(channel1));
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 4, channel2,sizeof(channel2));
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 5, channel3,sizeof(channel3));
      retval |= ctdbGetFieldAsString(hRecordSignalRecd, 6, channel4,sizeof(channel4));
      ctdbLock(hTableSignalRecd, CTLOCK_WRITE_BLOCK);

      if (retval)
         Handle_Error("Display_hRecordSignalRecd(): ctdbGetFieldAsString()");
      //determine usrno sequence
      if(usrno!=s_usrno || sequence!= s_sequence)
      {
    	  memset(buffer3,0,MSG_SIZE);
    	  strcpy(buffer3,"n");
    	  n=write(sendsock,buffer3,MSG_SIZE);
    	  printf("usrno end\n");
    	  break;
      }
      else
      {
          n=write(sendsock,buffer3,MSG_SIZE); //start send data
          read(sendsock,buffer2,MSG_SIZE);//block until server reply
      }


      memset(buffer,0,MSG_SIZE);
      strcpy(buffer,time);
      n=write(sendsock,buffer,MSG_SIZE); //send time
      read(sendsock,buffer2,MSG_SIZE);//block until server reply

      memset(buffer,0,MSG_SIZE);
      sprintf(buffer,"%s %s %s %s",channel1,channel2,channel3,channel4);
      n=write(sendsock,buffer,MSG_SIZE);
      read(sendsock,buffer2,MSG_SIZE);//block until server reply


      printf("\n\t\t%lu %lu %s %s %s %s %s\n",usrno,sequence,time,channel1,channel2,channel3,channel4);


      /* read prev record */
      retval = ctdbPrevRecord(hRecordSignalRecd);//because the index is decesding mode
      if (retval != CTDBRET_OK)
      {
    	  memset(buffer3,0,MSG_SIZE);
    	  strcpy(buffer3,"n");
    	  n=write(sendsock,buffer3,MSG_SIZE);
    	  //Handle_Error("Send_SignalRecd(): ctdbPrevRecord()");
      }
      i++;
   }
    memset(buffer3,0,MSG_SIZE);
    strcpy(buffer3,"n");
    n=write(sendsock,buffer3,MSG_SIZE);

}

/*read data option*/
void readdata(int sock,int usrno,int sequence)
{
	char buffer[MSG_SIZE];
	TEXT starttime[13+1];
	TEXT endtime[13+1];
	TEXT send_start[13+1];
	int send_num;

	//read connect record;
	ctdbLock(hTableConnRecd, CTLOCK_SUSPEND);
	if(ctdbClearRecord(hRecordConnRecd))
			Handle_Error("update");
	if(ctdbSetDefaultIndex(hRecordConnRecd,1))
		Handle_Error("readdata():ctdbSetDefaultIndex");
	if(ctdbSetFieldAsSigned(hRecordConnRecd,0,usrno))
			Handle_Error("readdata():ctdbSetFieldAsSigned");
	if(ctdbSetFieldAsSigned(hRecordConnRecd,1,sequence))
			Handle_Error("readdata():ctdbSetFieldAsSigned");
	if(ctdbFindRecord(hRecordConnRecd,CTFIND_EQ)!=CTDBRET_OK)
	{ 
		memset(buffer,0,MSG_SIZE);
		strcpy(buffer,"n");
		printf("no record\n");  //no record;
		write(sock,buffer,MSG_SIZE);
	}
	else//record exist
	{
	  ctdbGetFieldAsString(hRecordConnRecd, 3, starttime, sizeof(starttime));
	  ctdbGetFieldAsString(hRecordConnRecd, 4, endtime, sizeof(endtime));
	  //send back the start and end time
		memset(buffer,0,MSG_SIZE);
		sprintf(buffer,"%s,%s",starttime,endtime);
		write(sock,buffer,MSG_SIZE);

		memset(buffer,0,MSG_SIZE);
		read(sock,buffer,MSG_SIZE); //read the time and range;

		//process the string
		sscanf(buffer,"%s%d",send_start,&send_num);
		printf("%s,%d,%d,%d\n",send_start,strlen(send_start),sizeof(send_start),send_num);

	}
	ctdbLock(hTableConnRecd, CTLOCK_WRITE_BLOCK);
	//read signal
	ctdbLock(hTableSignalRecd, CTLOCK_SUSPEND);
	if(ctdbClearRecord(hRecordSignalRecd))
		Handle_Error("readdata():ctdbClearRecord");
	if(ctdbSetDefaultIndex(hRecordSignalRecd,1))
		Handle_Error("readdata():ctdbSetDefaultIndex");
	if(ctdbSetFieldAsSigned(hRecordSignalRecd,0,usrno))
		Handle_Error("readdata():ctdbSetFieldAsSigned");
	if(ctdbSetFieldAsSigned(hRecordSignalRecd,1,sequence))
		Handle_Error("readdata():ctdbSetFieldAsSigned");
	if(ctdbSetFieldAsString(hRecordSignalRecd,2,send_start))
		Handle_Error("readdata():ctdbSetFieldAsStringhalo");
	if(ctdbFindRecord(hRecordSignalRecd,CTFIND_GE)!=CTDBRET_OK)
	{ //no record
		ctdbLock(hTableSignalRecd, CTLOCK_WRITE_BLOCK);
		printf("record not found\n");
		memset(buffer,0,MSG_SIZE);
		strcpy(buffer,"n");
		printf("no record\n");  //no record;
		write(sock,buffer,MSG_SIZE);

	}
	else
	{
		ctdbLock(hTableSignalRecd, CTLOCK_WRITE_BLOCK);
		memset(buffer,0,MSG_SIZE);
		strcpy(buffer,"y");
		write(sock,buffer,MSG_SIZE);//confirm the monitoring program
		Send_SignalRecd(hRecordSignalRecd,sock,send_num,usrno,sequence); //send signal
	}


}

/*write data function*//
void writedata(int sock)
{
	int flag=1;
	int n,i=0;
	char buffer[MSG_SIZE]; //buffer receive data
	char buffer2[MSG_SIZE]; //buffer send confirm message
	char *recv_date;//start monitor date
	int recv_usrno; //user no
	int sequence=0;
	SignalRecd_DATA data3;
	char c1[MSG_SIZE],c2[MSG_SIZE],c3[MSG_SIZE],c4[MSG_SIZE],c5[MSG_SIZE]; //split string

	//read the first signal
	bzero(buffer,MSG_SIZE);
	read(sock,buffer,MSG_SIZE);
	bzero(buffer2,MSG_SIZE);
	strcpy(buffer2,buffer);
	recv_date=strtok(buffer2,",");//"," as the demarcation
	recv_usrno=atoi(strtok(NULL,","));

	if(check_usrno(recv_usrno)==0)//no userno
	{

		memset(buffer,0,MSG_SIZE);
		strcat(buffer,"stop");
		write(sock,buffer,strlen(buffer)); //stop end connection
		printf("no user\n");
		flag=0;
	}
	else
	{
		memset(buffer,0,MSG_SIZE);
		strcat(buffer,"go");
		n=write(sock,buffer,MSG_SIZE); //start transfer
		sequence=check_sequence(recv_usrno);
		printf("sequence:%d\n",sequence);
		Add_ConnRecd_Records(recv_date, recv_usrno,sequence+1);
		//Display_ConnRecd();

		data3.usrno=recv_usrno;
		data3.sequence=sequence+1;

		memset(buffer2,0,MSG_SIZE);
		strcpy(buffer2,"go");


		while(flag==1)
		{

		 	memset(buffer,0,MSG_SIZE);
		  	n=read(sock,buffer,MSG_SIZE);
		  	if(n<=0) //stop connection
		  	{
		  	   flag=0;
			   	 update_ConnRecd(data3.time,4,data3.usrno,data3.sequence);//update time according to 
			   	 																													//the last data
			   	 break;

		  	}
		    write(sock,buffer2,MSG_SIZE); //tell client go
		    sscanf(buffer,"%s%s%s%s%s",c1,c2,c3,c4,c5);//deal with the buffer
		    data3.time=c1;
		    data3.channel1=c2;
		    data3.channel2=c3;
		    data3.channel3=c4;
		    data3.channel4=c5;
		    Add_SignalRecd_Records(data3);//add data

		    if(i==0)
		    {
			    update_ConnRecd(data3.time,3,data3.usrno,data3.sequence);//update time according to 
			    																													//the first data
		    }
		    i++;
		}
	}
}

void dostuff (int sock)
{
   char buffer[MSG_SIZE];
   char temp[MSG_SIZE];
   int usrno,sequence;
   ioctl(sock,FIONBIO,0);
   int n;
   bzero(buffer,MSG_SIZE);
   n=read(sock,buffer,MSG_SIZE); //read the option: read or write
   sscanf(buffer,"%s%d%d",temp,&usrno,&sequence);
   printf("%s,%d,%d\n",temp,usrno,sequence);
   if (strcmp(buffer,"write")==0)
   {
	   writedata(sock); //write function
   }
   else
   {
	   readdata(sock,usrno,sequence); //read funtion
   }

}

