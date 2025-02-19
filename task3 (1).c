//TO MOVE USE 'A' FOR LEFT AND 'D' FOR RIGHT 

#define MPCORE_PRIV_TIMER 0xFFFEC600
//we initialize this array to ensure that only one block is falling from a column at a time
int column_occupied[5] = {0,0,0,0,0};

//define my object struct 
typedef struct{
	int column; 
	int speed;
	int alive;
	int current_y;
}Object; 

//we want to make an array of object structs

Object objects[3];




// Reads a byte from a specific memory address
char read_byte(unsigned int address){
	char value;
	__asm__ __volatile__(
		"ldrb %0, [%1]"
		:"=r"(value) //output operand
		:"r"(address) //input operand 
		);	
	return value;
}

// Writes a byte to a specific memory address
void write_byte(unsigned int address, char value){
	__asm__ __volatile__(
		"strb %0, [%1]"
		: //no output operand 
		:"r"(value), "r"(address) //input operands
	);
}

// Reads a halfword (2 bytes) from a specific memory address
short read_halfword(unsigned int address){
	char value;
	__asm__ __volatile__(
		"ldrh %0, [%1]"
		:"=r"(value) //output operand
		:"r"(address) //input operand 
		);
	return value;
}

// Writes a halfword (2 bytes) to a specific memory address
void write_halfword(unsigned int address, short value){
	__asm__ __volatile__(
		"strh %0, [%1]"
		: //no output operand 
		:"r"(value), "r"(address) //input operands
	);
}

// Reads a word (4 bytes) from a specific memory address
int read_word(unsigned int address){
	char value;
	__asm__ __volatile__(
		"ldr %0, [%1]"
		:"=r"(value) //output operand
		:"r"(address) //input operand 
		);
	return value;
}

// Writes a word (4 bytes) to a specific memory address
void write_word(unsigned int address, int value){
	__asm__ __volatile__(
		"str %0, [%1]"
		: //no output operand 
		:"r"(value), "r"(address) //input operands
	);
}
void VGA_draw_point(int x, int y, short c) {
	if (x >= 0 && x <= 319 && y >= 0 && y <= 239){
		unsigned int address = 0xc8000000 | (y << 10) | (x << 1);
		write_halfword(address, c);
		}
}
void VGA_clear_pixelbuff() {
	for (int x = 0; x <= 319;  x++){
		for (int y = 0; y <= 239; y++){
			VGA_draw_point(x,y,0);
		}
	}
}

void VGA_write_char(int x, int y, char c) {
	if (x >= 0 && x <= 79 && y >= 0 && y <= 59){
		unsigned int address = 0xc9000000 | (y << 7) | x;
		write_byte(address, c);
	}
}

void VGA_clear_charbuff() {
	for (int x = 0; x <= 79; x++){
		for (int y = 0; y <= 59; y++){
			VGA_write_char(x,y,0);
		}
	}
}

int read_PS2_data(char *data) {
	volatile int RVALID = ((*(volatile int *)0xff200100) >> 15) & 0x1;
	if (RVALID){
		//if RVALID is 1, we want the data pointer to contain the low 8 bits of RVALID
		*data = *((volatile int *)0xff200100) & 0xff; 
		return 1;
	}
	return 0;	
}

void write_hex_digit(unsigned int x,unsigned int y, char c) {
    if (c > 9) {
        c += 55;
    } else {
        c += 48;
    }
    c &= 255;
    VGA_write_char(x,y,c);
}
void write_byte_kbrd(unsigned int x,unsigned int y, unsigned int c) {
   char lower=c>>4 &0x0F;
   write_hex_digit(x,y,lower);
   char upper=c&0x0F;
   write_hex_digit(x+1,y,upper);
   return;
}

void VGA_fill(){
	for (int x = 0; x <= 319;  x++){
		for (int y = 0; y <= 239; y++){
			VGA_draw_point(x,y,0x041F);
		}
	}
}
void draw_character(int x) {
	//Im drawing a rectange thats the size of one grid cell
	//(320/5) x (240/5)
    int character_width = 64;
	int character_height = 48;
	short character_colour = 0;
	for (int ix = x-character_width/2; ix< x + character_width/2; ix++){
		for (int iy = 240; iy > 240 - character_height; iy--){
			VGA_draw_point(ix, iy, character_colour);
		}
	}
}

unsigned int seed = 12345;  // You can set this to any starting value

// Function to generate a pseudo-random number
unsigned int pseudo_random() {
    // LCG parameters (from Numerical Recipes)
    seed = (1103515245 * seed + 12345) & 0x7fffffff;
    return seed;
}

// Function to get a pseudo-random number within a specific range [min, max]
unsigned int random_in_range(int min, int max) {
    return (pseudo_random() % (max - min + 1)) + min;
}

void init_objects() {
	//a function that initializes all of our objects to alive. The
	//alive/not alive ensures we only have 5 items on screen at a time
    for (int i = 0; i <= 2; i++) {
        objects[i].alive = 0; 
		column_occupied[i] = 0;
    }
	column_occupied[3] = 0;
	column_occupied[4] = 0;
}



void draw_object(int x, int y){
	int object_width = 64;
	int object_height = 48;
	//red colour
	short object_colour = 0xF800;
	for (int ix = x - object_width/2; ix < x + object_width/2; ix++){
		for (int iy = y - object_height/2; iy < y + object_height/2; iy++){
			if (ix >= 0 && ix < 320 && iy >= 0 && iy < 240){
				VGA_draw_point(ix, iy, object_colour);
			}
		}
	}
}

void spawn_object(){
	//bounds are 0 to 2 because we picked the object struct array to have 
	//3 total objects
	for(int i = 0; i <= 2; i++){
		//we want to check which object slot is free and if it is spawn that object
		if (!(objects[i].alive)){
			objects[i].column = random_in_range(0,4);
			while (column_occupied[objects[i].column] == 1) {
                objects[i].column = random_in_range(0, 4);
            }
			objects[i].current_y = 0;
			objects[i].speed = random_in_range(20,25);
			column_occupied[objects[i].column]=1;
			objects[i].alive = 1;
			//because its a 5 cell grid, and the draw_object takes the center 
			//of the object, we draw at column*64 + 32
			draw_object(objects[i].column * 64 + 32, objects[i].current_y);
			break;
		}
	}
}


void erase_object(int x, int y){
	short background_colour = 0x041F;
	int object_width = 64;
	int object_height = 48;
	
	//same logic as draw_object, except we're writing in the 
	//background colour 
	for (int ix = x - object_width/2; ix < x + object_width/2; ix++){
		for (int iy = y - object_height/2; iy < y + object_height/2; iy++){
			if (ix >= 0 && ix < 320 && iy >= 0 && iy < 240){
				VGA_draw_point(ix, iy, background_colour);
			}
		}
	}
}

void update_objects(int *game_over_flag){
	for (int i=0; i<=2; i++){
		if(objects[i].alive){
			//erase the object in the current position first 
			erase_object(objects[i].column*64+32, objects[i].current_y);
			objects[i].current_y += objects[i].speed;
			
			//right now Im checking for 216 because 
			//we're drawing from the middle of the rectangle
			if(objects[i].current_y >= 240-24){
				*game_over_flag = 1;
				objects[i].alive = 0;
				column_occupied[objects[i].column]=0;
				break;
			}
			else{
				draw_object(objects[i].column*64+32, objects[i].current_y);
			}
		}
	}
}


int timer_expired(){
	volatile int *timer = (int *)MPCORE_PRIV_TIMER;
	if (*(timer + 3) & 1){
		*(timer+3) = 1;
		return 1;
	}
	return 0;
}


void erase_character(int x){
	short background_colour = 0x041F;
	int character_width = 64;
	int character_height = 48;
	
	//same logic as erase object except we're erasing the character
	for (int ix = x - character_width/2; ix < x + character_width/2; ix++){
		for (int iy = 216 - character_height/2; iy < 216 + character_height/2; iy++){
			if (ix >= 0 && ix < 320 && iy >= 0 && iy < 240){
				VGA_draw_point(ix, iy, background_colour);
			}
		}
	}
}
					 
void update_character(int *x, char key) {
    int character_width = 64;

    erase_character(*x);

    //move left if A is pressed and not at the left edge
    if (key == 0x1C && *x > character_width / 2){
        *x -= 64;
    }
    //move right if D is pressed and not at the right edge
    else if (key == 0x23 && *x < 320 - character_width / 2) {
        *x += 64;
    }


    draw_character(*x);
}


void update_score(int *score) {
    (*score)++;
}

void check_collision(int player_x, int *score) {
    for (int i = 0; i <=2; i++) {
        if (objects[i].alive) {
			//this is -24 because we're looking from the middle of the object 
            if (objects[i].current_y >= 240-48-24) {

                int object_center_x = objects[i].column * 64 + 32;
                if (object_center_x >= player_x - 32 && object_center_x <= player_x + 32) {
                    erase_object(object_center_x, objects[i].current_y);
                    objects[i].alive = 0;
					column_occupied[objects[i].column]=0;
                    update_score(score);
					draw_character(player_x);
                }
            }
        }
    }
}
void init_game(){
	VGA_fill();
	VGA_clear_charbuff();
	init_objects();
	draw_character(160);
	int *score = 0;
	volatile int *timer_ptr = (int *)MPCORE_PRIV_TIMER;
    int timeout = 100000000;     

    *(timer_ptr) = timeout;
    *(timer_ptr + 2) = 0b011;
	
}

void game_over(int score){
	VGA_fill();

	int x = 35;
	int y = 20;
	const char *message = "GAME OVER";
    
    for (int i = 0; message[i] != '\0'; i++) {
        VGA_write_char(x++, y, message[i]);
    }
	x = 35;
    y = 22;

    char score_text[20];
    snprintf(score_text, sizeof(score_text), "Score: %d", score);


    for (int i = 0; score_text[i] != '\0'; i++) {
        VGA_write_char(x++, y, score_text[i]);
    }
	
	
	//stall so that no incoming input from the last game starts the game
	
	char data;
	while(1){
		if (!(read_PS2_data(&data))){
			break;
		}
	}
	while (1) {
        if (read_PS2_data(&data)) {
            if (data == 0xF0) {
                break;
            }
        }
    }
}




int main() {
	char data;
    while (1) {
        if (read_PS2_data(&data)) {
	            break;
        }
    }
    while (1) {
        init_game();
		init_objects();
        int character_x = 160;
        int score = 0;
        char key;
		int spawn_timer = random_in_range(2, 4);

        


        int game_over_flag = 0;
        while (!game_over_flag) {
			if (read_PS2_data(&key)) {
				if (key == 0x1C || key == 0x23) {
					update_character(&character_x, key);
					}
				while(1){
						if (!read_PS2_data(&key)){
							break;
						}
				}
			}

			if (timer_expired()) {
				if (spawn_timer > 0) {
                    spawn_timer--;
                }
				if (spawn_timer == 0){
					spawn_object();
					spawn_timer = random_in_range(2,4);
				}
				update_objects(&game_over_flag);
				check_collision(character_x, &score);

			}
		}
        game_over(score);
    }

    return 0;
}
