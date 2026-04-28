#include <xc.h>
#include <stdint.h>

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 20000000UL

#define RS RD0
#define EN RD1

#define BTN_CHECK PORTBbits.RB0
#define BTN_ENROLL PORTBbits.RB1
#define BTN_DELETE PORTBbits.RB2

#define LED_GREEN PORTCbits.RC0
#define LED_RED PORTCbits.RC1

#define CMD_GENIMG   0x01
#define CMD_IMG2TZ   0x02
#define CMD_SEARCH   0x04
#define CMD_REGMODEL 0x05
#define CMD_STORE    0x06
#define CMD_DELETE   0x0C

#define ACK_SUCCESS 0x00
#define ACK_NOFINGER 0x02

void system_init(void);
void lcd_init(void);
void lcd_cmd(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_string(const char *s);
void lcd_clear(void);
void lcd_set_cursor(unsigned char row, unsigned char col);

void uart_set_57600(void);
void uart_set_9600(void);
void uart_write(unsigned char b);
int uart_read_tmo(unsigned char *out, unsigned int timeout_ms);
void uart_flush(void);

void servo_init(void);
void servo_angle(unsigned char angle);
void door_unlock(void);
void door_lock(void);

unsigned char as608_send_cmd(unsigned char cmd, unsigned char *params, unsigned char len);
unsigned char as608_send_cmd_with_reply(unsigned char cmd, unsigned char *params, unsigned char len, unsigned char *reply_data, unsigned char *reply_len);
unsigned char as608_get_image(void);
unsigned char as608_img2tz(unsigned char bufid);
unsigned char as608_search(unsigned char bufid, unsigned int *match_id, unsigned int *match_score);
unsigned char as608_create(void);
unsigned char as608_store(unsigned char bufid, unsigned int id);
unsigned char as608_delete(unsigned int id);

void show_menu(void);
void check_fingerprint(void);
void enroll_fingerprint(void);
void delete_fingerprint(void);

void main(void) {
    unsigned char resp;
    system_init();
    lcd_clear();

    lcd_set_cursor(1,1);
    lcd_string("F-22 RAPTOR");
    lcd_set_cursor(2,1);
    lcd_string("SECURITY SYS");
    __delay_ms(1800);

    lcd_clear();
    lcd_set_cursor(1,1);
    lcd_string("Detecting AS608");
    lcd_set_cursor(2,1);
    lcd_string("Try 57600...");
    uart_set_57600();
    __delay_ms(200);
    resp = as608_get_image();
    if(resp == 0xFF || resp == 0xFE) {
        lcd_clear();
        lcd_set_cursor(1,1);
        lcd_string("Try 9600...");
        uart_set_9600();
        __delay_ms(200);
        resp = as608_get_image();
        if(resp == 0xFF || resp == 0xFE) {
            lcd_clear();
            lcd_set_cursor(1,1);
            lcd_string("AS608 No Reply");
            lcd_set_cursor(2,1);
            lcd_string("Check Power/Wires");
            __delay_ms(2000);
        } else {
            lcd_clear();
            lcd_set_cursor(1,1);
            if(resp == ACK_NOFINGER) lcd_string("AS608 OK 9600");
            else lcd_string("AS608 Resp 9600");
            __delay_ms(900);
        }
    } else {
        lcd_clear();
        lcd_set_cursor(1,1);
        if(resp == ACK_NOFINGER) lcd_string("AS608 OK 57600");
        else lcd_string("AS608 Resp 57600");
        __delay_ms(900);
    }

    show_menu();
    door_lock();

    while(1) {
        if(BTN_CHECK == 0) {
            __delay_ms(40);
            if(BTN_CHECK == 0) { while(BTN_CHECK==0) __delay_ms(10); check_fingerprint(); }
        }
        if(BTN_ENROLL == 0) {
            __delay_ms(40);
            if(BTN_ENROLL == 0) { while(BTN_ENROLL==0) __delay_ms(10); enroll_fingerprint(); }
        }
        if(BTN_DELETE == 0) {
            __delay_ms(40);
            if(BTN_DELETE == 0) { while(BTN_DELETE==0) __delay_ms(10); delete_fingerprint(); }
        }
        __delay_ms(20);
    }
}

void system_init(void) {
    ADCON1 = 0x06;
    TRISD = 0x00;
    TRISB = 0xFF;
    TRISCbits.TRISC0 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;

    PORTD = 0x00; PORTB = 0x00; PORTC = 0x00;
    OPTION_REGbits.nRBPU = 0;

    __delay_ms(200);
    lcd_init();
    uart_set_57600();
    servo_init();

    LED_GREEN = 0;
    LED_RED = 1;
    __delay_ms(200);
}

void lcd_init(void) {
    __delay_ms(40);
    RS = 0; EN = 0;
    PORTD = (PORTD & 0x0F) | 0x30; EN=1; __delay_ms(2); EN=0; __delay_ms(5);
    PORTD = (PORTD & 0x0F) | 0x30; EN=1; __delay_ms(2); EN=0; __delay_ms(5);
    PORTD = (PORTD & 0x0F) | 0x20; EN=1; __delay_ms(2); EN=0; __delay_ms(5);

    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01); __delay_ms(2);
}

void lcd_cmd(unsigned char cmd) {
    RS = 0;
    PORTD = (PORTD & 0x0F) | (cmd & 0xF0);
    EN = 1; __delay_us(50); EN = 0; __delay_ms(2);
    PORTD = (PORTD & 0x0F) | ((cmd << 4) & 0xF0);
    EN = 1; __delay_us(50); EN = 0; __delay_ms(2);
}

void lcd_data(unsigned char data) {
    RS = 1;
    PORTD = (PORTD & 0x0F) | (data & 0xF0);
    EN = 1; __delay_us(50); EN = 0; __delay_us(50);
    PORTD = (PORTD & 0x0F) | ((data << 4) & 0xF0);
    EN = 1; __delay_us(50); EN = 0; __delay_ms(2);
}

void lcd_string(const char *s) {
    while(*s) lcd_data(*s++);
}

void lcd_clear(void) {
    lcd_cmd(0x01); __delay_ms(2);
}

void lcd_set_cursor(unsigned char row, unsigned char col) {
    lcd_cmd((row==1)?(0x80 + col - 1):(0xC0 + col -1));
}

void uart_set_57600(void) {
    TXSTAbits.SYNC = 0;
    TXSTAbits.BRGH = 1;
    SPBRG = 20;
    RCSTAbits.SPEN = 1;
    TXSTAbits.TXEN = 1;
    RCSTAbits.CREN = 1;
    PIE1bits.RCIE = 0;
}

void uart_set_9600(void) {
    TXSTAbits.SYNC = 0;
    TXSTAbits.BRGH = 1;
    SPBRG = 129;
    RCSTAbits.SPEN = 1;
    TXSTAbits.TXEN = 1;
    RCSTAbits.CREN = 1;
    PIE1bits.RCIE = 0;
}

void uart_write(unsigned char b) {
    while(!PIR1bits.TXIF);
    TXREG = b;
}

int uart_read_tmo(unsigned char *out, unsigned int timeout_ms) {
    unsigned long loops = (unsigned long)timeout_ms * 40UL;
    for(unsigned long i = 0; i < loops; i++) {
        if(PIR1bits.RCIF) {
            if(RCSTAbits.OERR) { RCSTAbits.CREN = 0; RCSTAbits.CREN = 1; }
            *out = RCREG;
            return 1;
        }
        __delay_us(25);
    }
    return 0;
}

void uart_flush(void) {
    unsigned char d;
    while(PIR1bits.RCIF) { d = RCREG; (void)d; }
}

void servo_init(void) {
    TRISCbits.TRISC2 = 0;
    CCP1CON = 0x0C;
    PR2 = 249;
    T2CON = 0x06;
    CCPR1L = 8;
    __delay_ms(50);
}

void servo_angle(unsigned char angle) {
    if(angle > 180) angle = 180;
    unsigned int pwm_val = 50 + ((unsigned int)angle * 100UL) / 180UL;
    CCPR1L = (pwm_val >> 2) & 0xFF;
    CCP1CON = (CCP1CON & 0xCF) | ((pwm_val & 0x03) << 4);
    __delay_ms(400);
}

void door_unlock(void) { LED_GREEN = 1; LED_RED = 0; servo_angle(0); }
void door_lock(void)   { LED_GREEN = 0; LED_RED = 1; servo_angle(90); }

unsigned char as608_send_cmd_with_reply(unsigned char cmd, unsigned char *params, unsigned char len, unsigned char *reply_data, unsigned char *reply_len) {
    unsigned int checksum = 0;
    unsigned int pkg_len = len + 3;
    unsigned char header[9];
    unsigned char i;

    uart_flush();

    uart_write(0xEF); uart_write(0x01);
    uart_write(0xFF); uart_write(0xFF); uart_write(0xFF); uart_write(0xFF);
    uart_write(0x01);
    uart_write((pkg_len >> 8) & 0xFF);
    uart_write(pkg_len & 0xFF);
    uart_write(cmd);

    checksum = 0x01 + ((pkg_len >> 8) & 0xFF) + (pkg_len & 0xFF) + cmd;
    for(i=0;i<len;i++) { uart_write(params[i]); checksum += params[i]; }
    uart_write((checksum >> 8) & 0xFF);
    uart_write(checksum & 0xFF);

    for(i=0;i<9;i++) {
        if(!uart_read_tmo(&header[i], 500)) {
            return 0xFF;
        }
    }
    
    if(header[0] != 0xEF || header[1] != 0x01) return 0xFE;

    unsigned int resp_len = ((unsigned int)header[7] << 8) | header[8];
    
    unsigned char conf_code;
    if(!uart_read_tmo(&conf_code, 500)) return 0xFF;
    
    if(reply_data != NULL && resp_len > 1) {
        unsigned char data_len = (resp_len > 32) ? 32 : (resp_len - 3);
        for(i=0; i<data_len; i++) {
            if(!uart_read_tmo(&reply_data[i], 500)) break;
        }
        if(reply_len != NULL) *reply_len = i;
    }
    
    for(i=0; i<(resp_len-1); i++) {
        unsigned char tmp;
        if(!uart_read_tmo(&tmp, 500)) break;
    }
    
    return conf_code;
}

unsigned char as608_send_cmd(unsigned char cmd, unsigned char *params, unsigned char len) {
    return as608_send_cmd_with_reply(cmd, params, len, NULL, NULL);
}

unsigned char as608_get_image(void)  { 
    return as608_send_cmd(CMD_GENIMG, NULL, 0); 
}

unsigned char as608_img2tz(unsigned char bufid) { 
    unsigned char p[1] = {bufid}; 
    return as608_send_cmd(CMD_IMG2TZ, p, 1); 
}

unsigned char as608_search(unsigned char bufid, unsigned int *match_id, unsigned int *match_score) { 
    unsigned char p[5] = {bufid, 0x00, 0x00, 0x00, 0xA3};
    unsigned char reply[8];
    unsigned char reply_len = 0;
    
    unsigned char res = as608_send_cmd_with_reply(CMD_SEARCH, p, 5, reply, &reply_len);
    
    if(res == ACK_SUCCESS && reply_len >= 4) {
        if(match_id != NULL) {
            *match_id = ((unsigned int)reply[0] << 8) | reply[1];
        }
        if(match_score != NULL) {
            *match_score = ((unsigned int)reply[2] << 8) | reply[3];
        }
    }
    
    return res;
}

unsigned char as608_create(void) { 
    return as608_send_cmd(CMD_REGMODEL, NULL, 0); 
}

unsigned char as608_store(unsigned char bufid, unsigned int id) { 
    unsigned char p[3] = {bufid, (id>>8)&0xFF, id&0xFF}; 
    return as608_send_cmd(CMD_STORE, p, 3); 
}

unsigned char as608_delete(unsigned int id) { 
    unsigned char p[4] = {(id>>8)&0xFF, id&0xFF, 0x00, 0x01}; 
    return as608_send_cmd(CMD_DELETE, p, 4); 
}

void show_menu(void) {
    lcd_clear();
    lcd_set_cursor(1,1);
    lcd_string("1:Check 2:Enr");
    lcd_set_cursor(2,1);
    lcd_string("3:Delete");
}

void check_fingerprint(void) {
    unsigned char res = 0;
    unsigned int match_id = 0;
    unsigned int match_score = 0;
    
    lcd_clear(); 
    lcd_set_cursor(1,1); 
    lcd_string("Place Finger...");
    __delay_ms(400);

    for(unsigned char i=0;i<50;i++) {
        res = as608_get_image();
        if(res == ACK_SUCCESS) break;
        if(res == ACK_NOFINGER || res == 0xFF) { 
            __delay_ms(400); 
            continue; 
        }
        __delay_ms(400);
    }
    
    if(res != ACK_SUCCESS) {
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("No Finger!");
        __delay_ms(1200);
        show_menu(); 
        return;
    }
    
    lcd_set_cursor(2,1); 
    lcd_string("Processing...");
    __delay_ms(200);
    
    res = as608_img2tz(1);
    if(res != ACK_SUCCESS) { 
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Process Error");
        lcd_set_cursor(2,1);
        lcd_string("Try Again");
        __delay_ms(1500); 
        show_menu(); 
        return; 
    }
    
    lcd_clear();
    lcd_set_cursor(1,1);
    lcd_string("Searching...");
    __delay_ms(200);
    
    res = as608_search(1, &match_id, &match_score);
    
    if(res == ACK_SUCCESS) {
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Access Granted!");
        lcd_set_cursor(2,1); 
        lcd_string("ID:");
        lcd_data('0' + (match_id / 10));
        lcd_data('0' + (match_id % 10));
        lcd_string(" Score:");
        lcd_data('0' + (match_score / 100));
        lcd_data('0' + ((match_score / 10) % 10));
        
        door_unlock();
        
        for(unsigned char i=0;i<15;i++) __delay_ms(1000);
        
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Locking Door...");
        door_lock(); 
        __delay_ms(800);
    } else {
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Access Denied!");
        lcd_set_cursor(2,1);
        lcd_string("Not Registered");
        __delay_ms(1800);
    }
    
    show_menu();
}

void enroll_fingerprint(void) {
    unsigned char res = 0;
    lcd_clear(); 
    lcd_set_cursor(1,1); 
    lcd_string("Enroll Mode");
    lcd_set_cursor(2,1); 
    lcd_string("Place Finger...");
    __delay_ms(900);

    for(unsigned char i=0;i<50;i++) {
        res = as608_get_image();
        if(res == ACK_SUCCESS) break;
        __delay_ms(400);
    }
    if(res != ACK_SUCCESS) { 
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Failed Img1"); 
        __delay_ms(1200); 
        show_menu(); 
        return; 
    }
    
    lcd_clear(); 
    lcd_set_cursor(1,1); 
    lcd_string("Got Image 1"); 
    __delay_ms(700);
    
    res = as608_img2tz(1); 
    if(res != ACK_SUCCESS) { 
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Process1 Err"); 
        __delay_ms(1200); 
        show_menu(); 
        return; 
    }

    lcd_clear(); 
    lcd_set_cursor(1,1); 
    lcd_string("Remove Finger"); 
    __delay_ms(1400);
    
    for(unsigned char i=0;i<100;i++) {
        res = as608_get_image();
        if(res == ACK_NOFINGER) break;
        __delay_ms(200);
    }

    lcd_clear(); 
    lcd_set_cursor(1,1); 
    lcd_string("Place Again..."); 
    __delay_ms(800);
    
    for(unsigned char i=0;i<50;i++) { 
        res = as608_get_image(); 
        if(res == ACK_SUCCESS) break; 
        __delay_ms(400); 
    }
    
    if(res != ACK_SUCCESS) { 
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Failed Img2"); 
        __delay_ms(1200); 
        show_menu(); 
        return; 
    }
    
    lcd_clear(); 
    lcd_set_cursor(1,1); 
    lcd_string("Got Image 2"); 
    __delay_ms(700);
    
    res = as608_img2tz(2); 
    if(res != ACK_SUCCESS) { 
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Process2 Err"); 
        __delay_ms(1200); 
        show_menu(); 
        return; 
    }

    lcd_clear(); 
    lcd_set_cursor(1,1); 
    lcd_string("Creating...");
    res = as608_create(); 
    if(res != ACK_SUCCESS) { 
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Match Failed"); 
        __delay_ms(1200); 
        show_menu(); 
        return; 
    }

    lcd_set_cursor(2,1); 
    lcd_string("Storing...");
    res = as608_store(1, 1);
    
    if(res == ACK_SUCCESS) { 
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("SUCCESS!"); 
        lcd_set_cursor(2,1); 
        lcd_string("ID:1 Saved"); 
        __delay_ms(1600); 
    } else { 
        lcd_clear(); 
        lcd_set_cursor(1,1); 
        lcd_string("Store Failed!"); 
        __delay_ms(1400); 
    }
    
    show_menu();
}

void delete_fingerprint(void) {
    lcd_clear(); 
    lcd_set_cursor(1,1); 
    lcd_string("Deleting ID:1"); 
    __delay_ms(400);
    
    unsigned char res = as608_delete(1);
    lcd_set_cursor(2,1);
    
    if(res == ACK_SUCCESS) {
        lcd_string("Deleted!");
    } else {
        lcd_string("Failed!");
        if(res == 0xFF) {
            __delay_ms(400);
            lcd_clear(); 
            lcd_set_cursor(1,1);
            lcd_string("No Reply - Check");
            lcd_set_cursor(2,1);
            lcd_string("Power / TX-RX");
            __delay_ms(1500);
        }
    }
    
    __delay_ms(900);
    show_menu();
}
