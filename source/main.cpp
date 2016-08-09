#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <malloc.h>
#include <regex>
#include <streambuf>
#include <cerrno>

#include <typeinfo>
#include <cmath>
#include <numeric>
#include <iterator>
#include <algorithm>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <3ds.h>

#include "tinyxml2.h"
#include "utils.h"
#include "data.h"
#include "menu.h"


static const u16 top = 0x140;
static std::string region = "ALL";
static bool bExit = false;


std::string upper(std::string s)
{
  std::string ups;
  
  for(unsigned int i = 0; i < s.size(); i++)
  {
    ups.push_back(std::toupper(s[i]));
  }
  
  return ups;
}

struct display_item {
  int ld;
  int index;
};

bool compareByLD(const display_item &a, const display_item &b)
{
    return a.ld < b.ld;
}

bool FileExists (std::string name){
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

int mkpath(std::string s,mode_t mode)
{
    size_t pre=0,pos;
    std::string dir;
    int mdret = 0;
    if(s[s.size()-1]!='/'){
        s+='/';
    }
    while((pos=s.find_first_of('/',pre))!=std::string::npos){
        dir=s.substr(0,pos++);
        pre=pos;
        if(dir.size()==0) continue; // if leading / first time is 0 length
        if((mdret=mkdir(dir.c_str(),mode)) && errno!=EEXIST){
            return mdret;
        }
    }
    return mdret;
}

char parse_hex(char c)
{
    if ('0' <= c && c <= '9') return c - '0';
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    std::abort();
}

char* parse_string(const std::string & s)
{
    char* buffer = new char[s.size() / 2];
    for (std::size_t i = 0; i != s.size() / 2; ++i)
        buffer[i] = 16 * parse_hex(s[2 * i]) + parse_hex(s[2 * i + 1]);
    return buffer;
}

void CreateTicket(std::string titleId, std::string encTitleKey, char* titleVersion, std::string outputFullPath)
{
    std::ofstream ofs;
    ofs.open(outputFullPath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    ofs.write(tikTemp, 0xA50);
    ofs.close();
    ofs.open(outputFullPath, std::ofstream::out | std::ofstream::in | std::ofstream::binary);
    ofs.seekp(top+0xA6, std::ios::beg);
    ofs.write(titleVersion, 0x2);
    ofs.seekp(top+0x9C, std::ios::beg);
    ofs.write(parse_string(titleId), 0x8);
    ofs.seekp(top+0x7F, std::ios::beg);
    ofs.write(parse_string(encTitleKey), 0x10);
    ofs.close();
}



void removeForbiddenChar(std::string* s)
{
    std::string::iterator it;
    std::string illegalChars = "\\/:?\"<>|";
    for (it = s->begin() ; it < s->end() ; ++it){
        bool found = illegalChars.find(*it) != std::string::npos;
        if(found)
        {
            *it = ' ';
        }
    }
}

std::string ToHex(const std::string& s)
{
    std::ostringstream ret;
    for (std::string::size_type i = 0; i < s.length(); ++i)
    {
        int z = s[i]&0xff;
        ret << std::hex << std::setfill('0') << std::setw(2) << z;
    }
    return ret.str();
}


std::string get_file_contents(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    return(std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
  }
  throw(errno);
}

int util_compare_u64(const void* e1, const void* e2) {
    u64 id1 = *(u64*) e1;
    u64 id2 = *(u64*) e2;

    return id1 > id2 ? 1 : id1 < id2 ? -1 : 0;
}

std::vector<std::string> util_get_installed_tickets()
{
	std::vector<std::string> vTickets;
	Result res = 0;
    u32 ticketCount = 0;
    if(R_SUCCEEDED(res = AM_GetTicketCount(&ticketCount))) {
        u64* ticketIDs = (u64*) calloc(ticketCount, sizeof(u64));
        if(ticketIDs != NULL) {
            if(R_SUCCEEDED(res = AM_GetTicketList(&ticketCount, ticketCount, 0, ticketIDs))) {
                qsort(ticketIDs, ticketCount, sizeof(u64), util_compare_u64);
				char cur[34];
                for(u32 i = 0; i < ticketCount && R_SUCCEEDED(res); i++) {
					sprintf(cur,"%016llX", ticketIDs[i]);
					vTickets.push_back(cur);
				}
			}
		}
	}
	return vTickets;
}

void action_download()
{
	printf("Downloading 3ds.titlekeys.com page...");
	
	mkpath("/TIKdevil/", 0777);
	mkpath("/TIKdevil/tickets/", 0777);
	FILE *oh = fopen("/TIKdevil/fullpage", "wb");
	Result sres = DownloadFile("https://3ds.titlekeys.com/", oh, true);
	fclose(oh);
	if (sres != 0)
	{
		printf("Could not download file.\n");
	}
	
	printf("done!\n\n");
}

void action_table_strip()
{
	printf("Stripping table...\n");
	printf("  Phase 1...");
	std::string divBuffer = get_file_contents("/TIKdevil/fullpage");
	std::string buf = divBuffer.substr((divBuffer.find("</thead>")  + 8), (divBuffer.find("</table>") - divBuffer.find("</thead>")));
	int totSize = buf.size();
	int lastPrint=0;
	printf("done!\n\n");
	
	printf("   Phase 2...\n   ");
	while(buf.find("<button class=\"clipboard btn btn-info btn-sm\"><span class=\"glyphicon glyphicon-copy\"></span></button>") != std::string::npos)
	{
		int curPos = buf.find("<button class=\"clipboard btn btn-info btn-sm\"><span class=\"glyphicon glyphicon-copy\"></span></button>");
		buf.erase(buf.find("<button class=\"clipboard btn btn-info btn-sm\"><span class=\"glyphicon glyphicon-copy\"></span></button>"), 101);
		int progress=curPos*100/totSize;
		if((progress%10==0 and progress>lastPrint)or(progress==0 and lastPrint==0)){
			printf("%d%% ", progress);
			lastPrint = progress+1;
		}
	}
	printf("100%%\n   Done!\n\n    Phase 3...\n    ");
	lastPrint=0;
	while(buf.find("<button class=\"qr btn btn-info btn-sm\"><span class=\"glyphicon glyphicon-qrcode\"></span></button>") != std::string::npos)
	{
		int curPos = buf.find("<button class=\"qr btn btn-info btn-sm\"><span class=\"glyphicon glyphicon-qrcode\"></span></button>");
		buf.erase(buf.find("<button class=\"qr btn btn-info btn-sm\"><span class=\"glyphicon glyphicon-qrcode\"></span></button>"), 96);
		int progress=curPos*100/totSize;
		if((progress%10==0 and progress>lastPrint)or(progress==0 and lastPrint==0)){
			printf("%d%% ", progress);
			lastPrint = progress+1;
		}
	}
	printf("100%%\n    Done!\n\nWriting to file...");
	FILE *fp = fopen ("/TIKdevil/table", "w+");
	fprintf(fp, "%s", buf.c_str());
	fclose(fp);
	printf("done!\n\n");
}


void action_missing_tickets(std::vector<std::string> &vEncTitleKey, std::vector<std::string> &vTitleID, std::vector<std::string> &vTitleRegion, int &n, std::string regionFilter, bool del)
{
	printf("Checking for already installed tiks...\n");
	tinyxml2::XMLDocument xmlDoc;
    xmlDoc.LoadFile( "/TIKdevil/table" );
	
	std::vector<std::string> vNANDTiks = util_get_installed_tickets();
	
	tinyxml2::XMLElement * pTR = xmlDoc.FirstChildElement("tr");
	const char*  ctitleId = nullptr;
	const char*  cencTitleKey = nullptr;
	const char*  ctitleType = nullptr;
	const char*  ctitleName = nullptr;
	const char*  ctitleRegion = nullptr;
	
	std::string titleId;
	std::string encTitleKey;
	std::string titleRegion;
	std::string curTik;
	
	while (pTR != nullptr)
	{
		int i=1;
		tinyxml2::XMLElement * pTD = pTR->FirstChildElement("td");
		while (pTD != nullptr)
		{
			const char* sOut = pTD->GetText();
			switch (i)
			{
				case(1):
					ctitleId = sOut;
				break;
				case(3):
					cencTitleKey = sOut;
				break;
				case(4):
					ctitleType = sOut;
				break;
				case(5):
					ctitleName = sOut;
				break;
				case(6):
					ctitleRegion = sOut;
				break;
			}
			pTD = pTD->NextSiblingElement("td");
			i++;
		}
		if(del == false)
		{
			
			if(strcmp(regionFilter.c_str(), "ALL"))
			{
				if(ctitleId != NULL and cencTitleKey != NULL and ctitleName != NULL and (not(strcmp(ctitleType, "eShop/Application")) or not(strcmp(ctitleType, "DLC"))) and(not(strcmp(ctitleRegion, regionFilter.c_str()))))
				{
					titleId = ctitleId;
					bool found = false;
					for(unsigned int foo =0; foo < vNANDTiks.size(); foo++)
					{
						curTik = vNANDTiks.at(foo);
						std::transform(curTik.begin(), curTik.end(), curTik.begin(), ::tolower);
						if(titleId == curTik) 
						{
							found=true;				
						}
					}
					if(found==false)
					{
						n++;
						
						encTitleKey = cencTitleKey;
						encTitleKey.erase(remove_if(encTitleKey.begin(), encTitleKey.end(), isspace), encTitleKey.end());
						titleRegion = ctitleRegion;
						
						vTitleID.push_back(titleId);
						vEncTitleKey.push_back(encTitleKey);
						vTitleRegion.push_back(titleRegion);
					}
				}
			} else {
				if(ctitleId != NULL and cencTitleKey != NULL and ctitleName != NULL and (not(strcmp(ctitleType, "eShop/Application")) or not(strcmp(ctitleType, "DLC"))))
				{
					titleId = ctitleId;
					bool found = false;
					for(unsigned int foo =0; foo < vNANDTiks.size(); foo++)
					{
						curTik = vNANDTiks.at(foo);
						std::transform(curTik.begin(), curTik.end(), curTik.begin(), ::tolower);
						if(titleId == curTik) 
						{
							found=true;				
						}
					}
					if(found==false)
					{
						encTitleKey = cencTitleKey;
						encTitleKey.erase(remove_if(encTitleKey.begin(), encTitleKey.end(), isspace), encTitleKey.end());
						n++;
						vTitleID.push_back(titleId);
						vEncTitleKey.push_back(encTitleKey);
					}
				}
			}
		} else {
			u64 curr;
			if(strcmp(regionFilter.c_str(), "ALL"))
			{
				if(ctitleId != NULL and cencTitleKey != NULL and ctitleName != NULL and (not(strcmp(ctitleType, "eShop/Application")) or not(strcmp(ctitleType, "DLC"))) and(strcmp(ctitleRegion, regionFilter.c_str())))
				{
					n++;
					curr = strtoull(ctitleId, NULL, 16) ;
					AM_DeleteTicket(curr);
				}
			} else {
				if(ctitleId != NULL and cencTitleKey != NULL and ctitleName != NULL and (not(strcmp(ctitleType, "eShop/Application")) or not(strcmp(ctitleType, "DLC"))))
				{
					n++;
					curr = strtoull(ctitleId, NULL, 16) ;
					AM_DeleteTicket(curr);
				}
			}
		}
		pTR = pTR->NextSiblingElement("tr");
	}
	if(del==false)printf("Missing tickets: %d\n\n", n);
	else if(del==true)printf("Deleted tickets: %d\n\n", n);
}

void action_generate(std::vector<std::string> vEncTitleKey,std::vector<std::string> vTitleID, int index)
{
	printf("Generating missing tickets...\n  ");
	char titleVersion[2] = {0x00, 0x00};
	int genlastPrint = 0;
	for (unsigned int i =0; i < vTitleID.size(); i++)
	{
		CreateTicket(vTitleID.at(i), vEncTitleKey.at(i), titleVersion, "/TIKdevil/tickets/" + vTitleID.at(i) + ".tik");
		int genprogress = i*100/index;
		if((genprogress%10==0 and genprogress>genlastPrint)or(genprogress==0 and genlastPrint==0)){
			printf("%d%% ", genprogress);
			genlastPrint = genprogress+1;
		}
	}
	printf("100%%\n  Done!\n\n");
}



void action_install(std::vector<std::string> vTitleID, int index)
{
	printf("Installing Tickets...\n  ");
	
	Handle hTik;
	u32 writtenbyte;
	int instlastPrint = 0;
	for (unsigned int i =0; i < vTitleID.size(); i++)
	{
		std::string cID = "/TIKdevil/tickets/";
		cID.append(vTitleID.at(i));
		cID.append(".tik");
		AM_InstallTicketBegin(&hTik);
		std::string curr = get_file_contents(cID.c_str());
		FSFILE_Write(hTik, &writtenbyte, 0, curr.c_str(), 0x150000, 0);
		AM_InstallTicketFinish(hTik);
		int instprogress = i*100/index;
		if((instprogress%10==0 and instprogress>instlastPrint)or(instprogress==0 and instlastPrint==0)){
			printf("%d%% ", instprogress);
			instlastPrint = instprogress+1;
		}
	}
	printf("100%%\n  Done!\n\n");
}

void action_clean(std::vector<std::string> vTitleID)
{
	printf("Cleaning temp files...");
	for (unsigned int i =0; i < vTitleID.size(); i++)
	{
		std::string rem = "/TIKdevil/tickets/";
		rem.append(vTitleID.at(i));
		rem.append(".tik");
		remove(rem.c_str());
	}
	
	printf("done!\n");
}

void action_about()
{
    consoleClear();
	
	printf(CONSOLE_RED "\n\n\n  TIKdevil by Kyraminol\n\n" CONSOLE_RESET);
    printf("    Generate only missing tickets\n");
    printf("    and directly install them!\n\n\n");
    printf(CONSOLE_BLUE "  Special thanks to:\n");
	printf("   cearp, Drakia, steveice10, Mmcx125.\n" CONSOLE_RESET);
    wait_key_specific("\n\n  Press A to continue.\n", KEY_A);
}

void action_toggle_region()
{
	if(region == "ALL")region = "EUR"; 
	else if(region == "EUR") region = "USA";
	else if(region == "USA") region = "JPN";
	else if(region == "JPN") region = "ALL";
}

int action_getconfirm(){
	int ret = 0;
	char msg[32];
	/* Commenting out due to missing code handling other regions.
	sprintf(msg, "Region set to %s are you sure?", region.c_str());
	if(region=="ALL"){
	    const char *confirm[] = {
			"No! Return me to main menu.",
			"Yes, but select EUR.",
			"Yes, but select USA.",
			"Yes, but select JPN.",
			"ARR ARR SELECT THEM ALL."
		};
		char foo[2];
		while(ret==0)
		{
			sprintf(foo, " ");
			int result = menu_draw(msg, foo, 0, sizeof(confirm) / sizeof(char*), confirm);
			switch (result)
			{
				case 0:
					ret=-1; break;
				case 1:
					ret=1; region = "EUR"; break;
				case 2:
					ret=1; region = "USA"; break;
				case 3:
					ret=1; region = "JPN"; break;
				case 4:
					ret=1; break;
			}
			clear_screen(GFX_BOTTOM);
		}
		consoleClear();
	} else {
		const char *confirm[] = {
			"No! Return me to main menu.",
			"Yes, continue with my selection.",
			"Yes, but select EUR.",
			"Yes, but select USA.",
			"Yes, but select JPN.",
			"ARR ARR SELECT THEM ALL."
		};
		char foo[2];
		while(ret==0)
		{
			
			sprintf(foo, " ");
			int result = menu_draw(msg, foo, 0, sizeof(confirm) / sizeof(char*), confirm);
			switch (result)
			{
				case 0:
					ret=-1; break;
				case 1:
					ret=1; break;
				case 2:
					ret=1; region = "EUR"; break;
				case 3:
					ret=1; region = "USA"; break;
				case 4:
					ret=1; region = "JPN"; break;
				case 5:
					ret=1; region = "ALL"; break;
			}
			clear_screen(GFX_BOTTOM);
		}
		consoleClear();
	} */
	return ret;
}



void select_oneclick()
{
	consoleClear();
	int w = action_getconfirm();
	if(w<0)return;
	action_download();
	action_table_strip();
	std::vector<std::string> Keys;
	std::vector<std::string> IDs;
	std::vector<std::string> Regions;
	int n;
	action_missing_tickets(Keys, IDs, Regions, n, region, false);
	action_generate(Keys, IDs, n);
	action_install(IDs, n);
	action_clean(IDs);
	wait_key_specific("\n  Press A to continue.\n", KEY_A);
}



void select_removeout(){
	consoleClear();
	int w = action_getconfirm();
	if(w<0)return;
	printf("Deleting selected tickets...\n");
	std::vector<std::string> Keys;
	std::vector<std::string> IDs;
	std::vector<std::string> Regions;
	int n;
	action_missing_tickets(Keys, IDs, Regions, n, region, true);
	printf("Done!");
	wait_key_specific("\nPress A to continue.\n", KEY_A);
}


bool menu_main_keypress(int selected, u32 key, void*)
{
    if (key & KEY_A)
    {
        switch (selected)
        {
            case 0:
                select_oneclick();
            break;
			case 1:
				select_removeout();
			break;
			
			
            case 2:
                action_about(); break;
            case 3:
                bExit = true; break;
        }
        return true;
    }
    else if (key & KEY_L)
    {
        action_toggle_region();
        return true;
    }

    return false;
}

void menu_main()
{
    const char *options[] = {
		"Update your Tickets!",
		"Remove out-of-region tickets",
        "About TIKdevil",
        "Exit",
    };
    char footer[37];

    while (!bExit)
    {
		sprintf(footer, "Region: [%s] (Press L to change)", region.c_str());
		menu_multkey_draw("TIKdevil by Kyraminol", footer, 0, sizeof(options) / sizeof(char*), options, NULL, menu_main_keypress);
        clear_screen(GFX_BOTTOM);
    }
}

int main(int argc, const char* argv[])
{
    u32 *soc_sharedmem, soc_sharedmem_size = 0x100000;
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    httpcInit(0);
    soc_sharedmem = (u32 *)memalign(0x1000, soc_sharedmem_size);
    socInit(soc_sharedmem, soc_sharedmem_size);
    sslcInit(0);
	amInit();
    AM_InitializeExternalTitleDatabase(false);

    init_menu(GFX_TOP);
    menu_main();
	
	amExit();
    gfxExit();
    httpcExit();
    socExit();
    sslcExit();
}
