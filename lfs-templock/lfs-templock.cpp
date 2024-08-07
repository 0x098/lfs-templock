// what this do? 
// educate you.
#define WIN32_LEAN_AND_MEAN // hehe.. LEAN!
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#define uc unsigned char
#define cuc const uc
#define read ReadProcessMemory
#define wryt WriteProcessMemory
#define p std::cout
#define ptr uintptr_t

cuc toFind[] = { 0xD8, 0x4D, 0xDC, 0xD8, 0x65, 0x14, 0xD8 };
cuc toRepW[] = { 0x90, 0x90, 0x90, 0xD8, 0x65, 0x14, 0xD8 };

uc buffr[ sizeof( toFind ) ];
int main() {
  HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );

  PROCESSENTRY32 entr = { };
  entr.dwSize = sizeof( PROCESSENTRY32 );

  ptr pid = 0ull;
  void * pHandle = nullptr;

  while( Process32Next( snap, &entr ) ) {
    if( wcscmp( entr.szExeFile, L"LFS.exe" ) == 0 ) {
      pid = entr.th32ProcessID;
      pHandle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
      p << "found handle: " << pHandle << " pid: " << entr.th32ProcessID << "\n";
      break;
    }
  } CloseHandle( snap );

  HANDLE snap2 = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid );

  MODULEENTRY32 entr2 = { };
  entr2.dwSize = sizeof( MODULEENTRY32 );

  ptr lfsBA = 0;

  while( Module32Next( snap2, &entr2 ) ) {
    std::wcout << entr2.szModule << " : " << std::hex << entr2.modBaseAddr << "\n";
    if( wcscmp( entr2.szModule, L"LFS.exe" ) == 0 ) {
      lfsBA = ( ptr ) entr2.modBaseAddr;
      break;
    }
  } CloseHandle( snap2 );

  if( !pHandle )
    return 1;

  int matchOffset = 0;
  uc ourmem[ 2 << 14 ]{}; // 16mb

  //read( pHandle, theAddress, buffr, sizeof( toFind ), 0 );
  for( size_t memofmult = 0; memofmult < 128; memofmult++ ) {
    read( pHandle, ( LPCVOID ) (lfsBA + memofmult * sizeof(ourmem)), ourmem, sizeof(ourmem), 0);
    for( size_t i = 0; i < sizeof( ourmem ); i++ ) {
      if( ourmem[ i ] == toFind[ 0 ] or ourmem[ i ] == toRepW[ 0 ] ) {
        bool missedFirst = false;
        bool missedSecond = false;
        read( pHandle, ( LPCVOID ) ( lfsBA + i + memofmult * sizeof( ourmem ) ), buffr, sizeof( buffr ), 0 );
        for( size_t buffercounter = 0; buffercounter < sizeof( buffr ); buffercounter++ ) {
          if( buffr[ buffercounter ] != toFind[ buffercounter ] ) {
            missedFirst = true;
          }
          if( buffr[ buffercounter ] != toRepW[ buffercounter ] ) {
            missedSecond = true;
          }
        }
        if( !missedFirst || !missedSecond ) {
          matchOffset = i + memofmult * sizeof( ourmem );
          goto FIND;
        }
      }
    }
  }
  FIND:
  std::cout << "found match (?):" << std::hex << matchOffset << "\n";
  LPCVOID theAddress = ( LPCVOID ) ( lfsBA + matchOffset ); // hardcode (not anymore)

  char uinpt[ 2 ];
  for( size_t i = 0; i < sizeof( toFind ) - 1; i++ ) {
    if( toFind[ i ] != buffr[ i ] ) {
      p << "already written (or some other magic occurred)\nundo change?(y/n):";
      std::cin >> uinpt;
      switch( *uinpt ) {
        case 'y': case 'Y':
        p << uinpt << "\nproceeding to write...\n";
        wryt( pHandle, ( LPVOID ) theAddress, toFind, sizeof( toRepW ), 0 );
        break;
      }
      system( "pause" );
      return 3;
    }
  }
  p << "we good\nproceeding to write...\n";

  wryt( pHandle, ( LPVOID ) theAddress, toRepW, sizeof( toRepW ), 0 );
  system( "pause" );
  return 4;
}