#include "shared.hpp"
#include "Matrix.hpp"
#include "const.cpp"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define STEPSIZE 100

static bool debug;
static bool output;
static bool Running;



pthread_mutex_t mutex;
pthread_cond_t cond_entry;

DynamicDeque<struct Move> move_buffer;
DynamicDeque<int> path_tree_buffer;
DynamicDeque<int> path_history_buffer;

void BuildPath(int source, int dest, DynamicDeque<int>* buffer, Matrix<int,81,81> P)
{
  buffer->push_back(source);
  get_path(P, buffer, source, dest);
  buffer->push_back(dest);
}

void GeneratePathTree(struct Move move,
		      DynamicDeque<int>* path_tree_buffer,
		      Matrix<int,81,81> P,
		      u8* last_position)
{
  static const int lookup[100] = {0,1,2,3,4,5,6,7,8,8,
				  9,10,11,12,13,14,15,16,17,17,
				  18,19,20,21,22,23,24,25,26,26,
				  27,28,29,30,31,32,33,34,35,35,
				  36,37,38,39,40,41,42,43,44,44,
				  45,46,47,48,49,50,51,52,53,53,
				  54,55,56,57,58,59,60,61,62,62,
				  63,64,65,66,67,68,69,70,71,71,
				  72,73,74,75,76,77,78,79,80,80,
				  72,73,74,75,76,77,78,79,80,80
  };

  if(debug) PrintDebug("Generating route from: %d, to %d for the path tree.\n", move.source, move.destination);
  
  // Go from the last position to move.source.
  BuildPath(lookup[*last_position], lookup[move.source], path_tree_buffer, P);
  // This handles from the source to the destination
  path_tree_buffer->push_back(-1); // @HACK: We use -1 to indicate that we need to toggle the magnet.
  path_tree_buffer->push_back(move.source);

  // This check makes sure that we don't generate a path to ourself.
  if(lookup[move.source] != lookup[move.destination]) {
    BuildPath(lookup[move.source], lookup[move.destination], path_tree_buffer, P); // Build the actual move path.
  }
  
  path_tree_buffer->push_back(-1);
  path_tree_buffer->push_back(move.destination);

  
  *last_position = move.destination;
}

static bool OutputPathHistory(const char* filePath,
			      DynamicDeque<int>& path_history)
{
  File file;
  if(!file.Open(filePath, "wb")) {
    PrintError("Failed to open output file: %s\n", filePath);
    return false;
  }

  if(debug) PrintDebug("Writing data to file: %s\n", filePath);
  while(path_history_buffer.getnum()) {
    const u16 path_point = (u16)path_history_buffer.pop_back();
    file.Write(&path_point, sizeof(path_point));
  }

  PrintDebug("Finished outputting paths to: %s\n", filePath);

  return true;
}

void* pathfinding_thread(void* data)
{
  thread_info* info = (thread_info *)data;
  
  if(debug) {
    PrintDebug("Pathfinding thread started with ID: %d\n", info->LogicalThreadIndex);
  }

  Matrix<int, 81, 81> m(arr); // Variable: arr is defined in consts.
  auto P = floydWarshall(m);  // Make the predecessor matrix.

  struct Move move1 = { 13, 40};
  struct Move move2 = { 12, 33};
  struct Move move3 = { 22, 42};
  struct Move move4 = { 18, 88};
  struct Move move5 = { 88, 18};
  struct Move move6 = { -1, -1};
  move_buffer.push_back(move1);
  move_buffer.push_back(move2);
  move_buffer.push_back(move3);
  move_buffer.push_back(move4);
  move_buffer.push_back(move5);
  move_buffer.push_back(move6);
  
  // Define the different kind of pieces used on the chess board.
  enum {
		     BLACK_PAWN,
		     BLACK_KNIGHT,
		     BLACK_BISHOP,
		     BLACK_ROOK,
		     BLACK_QUEEN,
		     BLACK_KING,
		     WHITE_PAWN,
		     WHITE_KNIGHT,
		     WHITE_BISHOP,
		     WHITE_ROOK,
		     WHITE_QUEEN,
		     WHITE_KING,
		     EMPTY
  };

  static int chessboard[100] = {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
				EMPTY, BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_KING, BLACK_QUEEN, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK, EMPTY,
				EMPTY, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, EMPTY,
				EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
				EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
				EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
				EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
				EMPTY, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, EMPTY,
				EMPTY, WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_KING, WHITE_QUEEN, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK, EMPTY,
				EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY};

  // We use this next array to keep track of the default positions.
  // This is used when we need to reset the positions of our chessboard.
  static int default_state[100];
  memcpy(default_state, chessboard, sizeof(int)*100);
  
  u8 last_position = 0;
  static const u16 bench[37] = {0,1,2,3,4,5,6,7,8,9,10,20,30,40,50,60,70,80,90,91,92,93,94,95,96,97,98,99,19,29,39,49,59,69,79,89,99};
  u16 bench_index = 0;
  
  while(Running) {
    pthread_mutex_lock(&mutex);
    if (move_buffer.getnum()) {

      auto move = move_buffer.pop_back();

      // Handle the event where the move indicated we need to reset our board_state.
      if(move.source == -1) {

	if(debug) PrintDebug("Received reset condition.\n");

	for(int i = 0; i < 99; i++) {
	  if(chessboard[i] != default_state[i] && chessboard[i] != EMPTY) {
	      for(int j = 0; j < 99; j++) { // Loop through to find where this needs to go.
		if(default_state[j] == chessboard[i] && chessboard[j] == EMPTY) {
		  struct Move piece_to_default_position = { i, j };
		  GeneratePathTree(piece_to_default_position, &path_tree_buffer, P, &last_position);
		}
	      }
	  } 
	}

	if(debug) PrintDebug("Finished pathfinding the reset condition.\n");
	path_tree_buffer.push_back(-2); // @HACK: Push reset to the PSoC-thread.
	last_position = 0;
	memcpy(chessboard, default_state, sizeof(int)*100); // Reset the state of the board.
      } else {
	// If the move isn't a reset, handle it here.
	// We need to check if the move is to an empty square.
	if(chessboard[move.destination] != EMPTY) {
	  
	  // If the square is not empty we need to capture the piece first.
	  struct Move piece_to_bench = {move.destination, bench[bench_index++]};
	  GeneratePathTree(piece_to_bench, &path_tree_buffer, P, &last_position);
	  
	  // Update the board.
	  chessboard[piece_to_bench.destination] = chessboard[piece_to_bench.source];
	  chessboard[piece_to_bench.source] = EMPTY;
	}

	// Proceed wether or not the above case fired, and update the board.
	// Then generate the route from the source, to the destination.
	chessboard[move.destination] = chessboard[move.source];
	chessboard[move.source] = EMPTY;

	GeneratePathTree(move, &path_tree_buffer, P, &last_position);
      }
          
      pthread_cond_signal(&cond_entry);
    }
    pthread_mutex_unlock(&mutex);
  }
			  
  pthread_exit(NULL);
}

void* psoc_boundary_thread(void* data)
{
  thread_info* info = (thread_info *)data;

  // Define the message texts.
  static const char * msg[] = {
				     "mOn",
				     "mOff",
				     "reset"
  };

  if (debug) {
    PrintDebug("PSOC boundary thread started with ID: %d\n", info->LogicalThreadIndex);
  }

  File file;
  const char* filename = "output.txt"; // /dev/spi_drv0
  
  bool active = false; // Determines the state of the magnet.
  s16 x = 0;
  s16 y = 0;
  
  Buffer buf; // Used to get the output to print.
  
  while(Running) {
    pthread_mutex_lock(&mutex);
    
    while(!path_tree_buffer.getnum())
      pthread_cond_wait(&cond_entry, &mutex);
 
    file.Open(filename, "wb");

    if(debug) PrintDebug("Writing paths to %s.\n", filename);

    while(path_tree_buffer.getnum()) {

      auto node = path_tree_buffer.pop_back();

      switch(node) {
      case -1: // This routine moves in and back to the node it came from
	
        node = path_tree_buffer.pop_back(); // Get the next node
	
	if((unsigned int)(node - 90) <= (unsigned int)(98 - 90)) {
	  // The left most set of nodes.
	  if(!active) {
	    WriteStepsToFile(&file, x - (STEPSIZE/2), y + (STEPSIZE/2));
	    file.Write(msg[0], strlen(msg[0]));
	    active = true;
	    } else {
	    WriteStepsToFile(&file, -(STEPSIZE/2), (STEPSIZE/2));
	    file.Write(msg[1], strlen(msg[1]));active = false;
	  }
	  WriteStepsToFile(&file, (STEPSIZE/2), -(STEPSIZE/2));	  
	} else if((unsigned int)(node - 9) % 10 == 0 && node != 99) {
	  // The bottom set row of nodes (without 99).
	  if(!active) {
	    WriteStepsToFile(&file, x + (STEPSIZE/2), y - (STEPSIZE/2));
	    file.Write(msg[0], strlen(msg[0]));
	    active = true;
	    } else {
	    WriteStepsToFile(&file, (STEPSIZE/2), -(STEPSIZE/2));
	    file.Write(msg[1], strlen(msg[1]));active = false;}
	  WriteStepsToFile(&file, -(STEPSIZE/2), (STEPSIZE/2));
	} else if((unsigned int)node == 99) {
	  // The bottom most left node.
	  if(!active) {
	    WriteStepsToFile(&file, x + (STEPSIZE/2), y + (STEPSIZE/2));
	    file.Write(msg[0], strlen(msg[0]));
	    active = true;
	    } else {
	    WriteStepsToFile(&file, (STEPSIZE/2), (STEPSIZE/2));
	    file.Write(msg[1], strlen(msg[1]));active = false;}
	  WriteStepsToFile(&file, -(STEPSIZE/2), -(STEPSIZE/2));
	} else {
	  // Default.
	  if(!active) {
	    WriteStepsToFile(&file, x - (STEPSIZE/2), y - (STEPSIZE/2));
	    file.Write(msg[0], strlen(msg[0]));
	    active = true;
	    } else {
	    WriteStepsToFile(&file, -(STEPSIZE/2), -(STEPSIZE/2));
	    file.Write(msg[1], strlen(msg[1]));active = false;}
	  WriteStepsToFile(&file, (STEPSIZE/2), (STEPSIZE/2));
	}

	x = 0;
	y = 0;
	
	break;
      case -2:
	file.Write(msg[2], strlen(msg[2]));
	break;
      default: // The default case is a simple move between adjacent nodes.
	// The difference indicates in which way we need to move.	
	//auto next_node = path_tree_buffer.pop_back()
	
	auto diff = path_tree_buffer[0] - node;
	
	switch(diff) {
	case 9:
	  if(!active) {
	    y += STEPSIZE;
	  } else {
	    WriteStepsToFile(&file, 0, STEPSIZE);
	  }
	  break;
	case -9:
	  if(!active) {
	    y -= STEPSIZE;
	  } else {
	    WriteStepsToFile(&file, 0, -STEPSIZE);
	  }
	  break;
	case 1:
	  if(!active) {
	    x += STEPSIZE;
	  } else {
	    WriteStepsToFile(&file, STEPSIZE, 0);
	  }
	  break;
	case -1:
	  if(!active) {
	    x -= STEPSIZE;
	  } else {
	    WriteStepsToFile(&file, -STEPSIZE, 0);
	  }
	  break;
	}
     	break;
      }
    }
	
    file.Close();

    if(debug) {
      if(!ReadEntireFile(buf, filename)) {
	PrintError("Couldn't read the output file.\n");
      }
      PrintDebug("Reading output from psoc-thread to %s.\n", filename);
      fwrite(buf.buffer, buf.length, buf.length, stdout);
    }

    pthread_mutex_unlock(&mutex);
  }
  
  pthread_exit(NULL);
}

int main(int argc, char** argv)
{

  if (ShouldPrintHelp(argc, argv)) {
    printf("Performs pathfinding with received moves, and outputs them to a char-driver.\n");
    printf("\n");
    printf("%s [-debug=x] [-output <filename>]\n", GetExecutableFileName(argv[0]));
    printf("\n");
    printf("output   Determines whether or not the path history will get printed,\n");
    printf("         at the end of the game.\n");
    printf("         By default, this string is null.\n");
    printf("\n");
    printf("debug    Determines whether debug info gets written to the the terminal.\n");
    printf("         By default, this number is 0. Allowed range: [0,1].\n");
    return 0;
  }

  const char* outputPath;
  
  for(int i = 1; i < argc; ++i) {
    const char* const arg = argv[i];
    if(strstr(arg, "-debug=") == arg) {
      int s;
      if(sscanf(arg, "-debug=%d", &s) == 1 && s == 1) {
	debug = true;
      }
    }
    if(strstr(arg, "-output") == arg) {
      outputPath = argv[i+1];
      if(outputPath == NULL) {
	FatalError("Output requested but no filepath given.\n");
      }
    } else {
      outputPath = NULL; 
    }
  }
  
  // Create Pathfinding thread and PSOC-boundary thread.
  pthread_t path_thread, psoc_thread;
  thread_info info[2];

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond_entry, NULL);

  move_buffer.reserve(10);
  path_tree_buffer.reserve(100);
  path_history_buffer.reserve(256); // @TODO: Approximate the average amount of paths in a game?
                                    //        This way we could save memory, but how much do we need?

  Running = true;
  
  for (int i = 0; i < 2; i++) {
    switch(i) {
    case 0:
      info[i].LogicalThreadIndex = i;
      if (pthread_create(&path_thread, NULL, pathfinding_thread, &info[i]) != 0)
	{
	  FatalError("Failed when creating pathfinding thread.\n");
	}
      break;
    case 1:
      info[i].LogicalThreadIndex = i;
      if (pthread_create(&psoc_thread, NULL, psoc_boundary_thread, &info[i]) != 0)
	{
	  FatalError("Failed when creating PSOC-boundary thread.\n");
	}
      break;
    default:
      break;
    }
  }
  
  pthread_join(path_thread, NULL);
  pthread_join(psoc_thread, NULL);

  if(output) {
    if(!OutputPathHistory(outputPath, path_history_buffer))
      PrintError("Failed writing the path history to: %s\n", outputPath);
  }
    
  return 0;
}
