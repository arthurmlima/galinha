// Arthur Mendes Lima 180046004
// projeto lab sismic 2021 turma C
// plataforma inercial - A GALINHA
#include <msp430.h>

#include "math.h"


#define RAD_2_DEG             57.29578
#define TRUE 1
#define FALSE 0
#define BR10K 105 //(SMCLK) 1.048.576/105 ~= 10kHz
#define BR100K 11 //(SMCLK) 1.048.576/11 ~= 100kHz
// MPU-6050 algumas constantes
#define MPU_ADR 0x68 //Endereço I2C do MPU
#define MPU_WHO 0x68 //Resposta ao Who am I
#define INT_PIN_CFG 0x37
#define INT_ENABLE 0x38
#define INT_STATUS 0x3A
#define WHO_AM_I 0x75 //Registrador Who am I
#define SMPLRT_DIV 0x19
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define ACCEL_XOUT_H 0x3B
#define PWR_MGMT_1 0x6B
#define WHO_AM_I 0x75 //Registrador Who am I
#define gyro_lsb_to_degsec  16.4F
#define acc_lsb_to_g  2048.0
#define CALIB_VEZES  500


// Protótipo das funções

void mpu_config(void);
void mpu_rd_vet(char reg, char *vt, char qtd);
void mpu_wr(char reg, char dado);
char mpu_rd(char reg);
char i2c_test(char adr);
void USCI_B0_config(void);
void leds_s1_config(void);
void delay (int x);
void set_timer_millis(void);
void gpio_config(void);
void monitorS1(int *contadorS1,int *S1);
void monitorS2(int *contadorS2,int *S2);
void pwm_init(void);
void servo_desloca(int graus_deslocamento);
void servo_angulo(float anguloX,float anguloY);
void media_movel_angulo(float* anguloX, float* anguloY);
void servo_angulo_acc(float AccX, float AccY);
void servo_angulo_gyr_topo(float angulo);
void servo_angulo_gyr_base(float angulo);
void servo_angulo_acc_base(float Acc);
void servo_angulo_acc_topo(float Acc);

long contador_millis =0;
float vec_mpu6050[7]={0,0,0,0,0,0,0};

 int main(void){
    char who;
    char vetor[16];
    volatile int ax,ay,az,tp,gx,gy,gz;
    volatile float Aax,Aay,Aaz,Ggx,Ggy,Ggz;

    volatile float offset_gx=0;
    volatile float offset_gy=0;
    volatile float offset_gz=0;

    volatile float offset_ax=0;
    volatile float offset_ay=0;
    volatile float offset_az=0;

    float angleX=0;
    float angleY=0;
    float angleZ=0;



    volatile int long desloc_ang=0;

    volatile static float dt=0,ultimo_tempo=0;

    int contadorS1=0,contadorS2=0;
    int S1=0,S2=0;


    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer



    leds_s1_config(); //Configurar Leds e S1
    USCI_B0_config(); //Configurar USCI_B0

    // MPU responde ao endereçamento com ACK?
    if (i2c_test(MPU_ADR) == FALSE)
    {
            P1OUT |= BIT0; //Sinalizar problema
            while(TRUE); //Travar execução
    }

    UCB0I2CSA = MPU_ADR; //Endereço a ser usado
    // Leitura do registrador WHO AM I retorna o esperado?
    if (mpu_rd(WHO_AM_I) != MPU_WHO)
    {
        P1OUT |= BIT0; //Sinalizar problema
        while(TRUE); //Travar execução
    }

    mpu_config(); //Configurar MPU
    gpio_config();
    P4OUT |= BIT7; //Verde aceso = OK


    int nd =0;







    while(nd<CALIB_VEZES)
    {
    while((P1IN & BIT2) == 0);

    mpu_rd_vet(ACCEL_XOUT_H, vetor, 14); //Ler 14 regs

         ax=vetor[ 0]; ax=(ax<<8)+vetor[ 1]; //aceleração eixo x
         ay=vetor[ 2]; ay=(ay<<8)+vetor[ 3]; //aceleração eixo y
         az=vetor[ 4]; az=(az<<8)+vetor[ 5]; //aceleração eixo z

         tp=vetor[ 6]; tp=(tp<<8)+vetor[ 7]; //temperatura

         gx=vetor[ 8]; gx=(gx<<8)+vetor[ 9]; //giro eixo x
         gy=vetor[10]; gy=(gy<<8)+vetor[11]; //giro eixo y
         gz=vetor[12]; gz=(gz<<8)+vetor[13]; //giro eixo z

         offset_gx+=gx/gyro_lsb_to_degsec;
         offset_gy+=gy/gyro_lsb_to_degsec;
         offset_gz+=gz/gyro_lsb_to_degsec;

         offset_ax+=ax/acc_lsb_to_g;
         offset_ay+=ay/acc_lsb_to_g;
         offset_az+=az/acc_lsb_to_g;
         nd++;
    }

    offset_gx=offset_gx/CALIB_VEZES;
    offset_gy=offset_gy/CALIB_VEZES;
    offset_gz=offset_gz/CALIB_VEZES;

    offset_ax=offset_ax/CALIB_VEZES;
    offset_ay=offset_ay/CALIB_VEZES;
    offset_az=offset_az/CALIB_VEZES;

    pwm_init();
    P4OUT ^= BIT7;
    __delay_cycles(1000000);
    P4OUT ^= BIT7;

    set_timer_millis();
    __enable_interrupt();

    while (TRUE)
    {
        while((P1IN & BIT2)==0);

        mpu_rd_vet(ACCEL_XOUT_H, vetor, 14); //Ler 14 regs

        ax=vetor[ 0]; ax=(ax<<8)+vetor[ 1]; //aceleracao eixo x
        ay=vetor[ 2]; ay=(ay<<8)+vetor[ 3]; //aceleracao eixo y
        az=vetor[ 4]; az=(az<<8)+vetor[ 5]; //aceleracao eixo z

        tp=vetor[ 6]; tp=(tp<<8)+vetor[ 7]; //temperatura

        gx=vetor[ 8]; gx=(gx<<8)+vetor[ 9]; //giro eixo x
        gy=vetor[10]; gy=(gy<<8)+vetor[11]; //giro eixo y
        gz=vetor[12]; gz=(gz<<8)+vetor[13]; //giro eixo z


       vec_mpu6050[4]=(float)ax/acc_lsb_to_g - offset_ax ;
       vec_mpu6050[5]=(float)ay/acc_lsb_to_g - offset_ay ;
       vec_mpu6050[6]=(float)az/acc_lsb_to_g - offset_az ;


        Ggx=(float)gx/gyro_lsb_to_degsec - offset_gx ;
        Ggy=(float)gy/gyro_lsb_to_degsec - offset_gy ;
        Ggz=(float)gz/gyro_lsb_to_degsec - offset_gz ;


        dt= (contador_millis-ultimo_tempo)*0.001;
        ultimo_tempo=contador_millis;

        vec_mpu6050[2] +=  Ggx*dt;
        vec_mpu6050[1] +=  Ggy*dt;
        vec_mpu6050[3] +=  Ggz*dt;

        monitorS1(&contadorS1,&S1);
        monitorS2(&contadorS2,&S2);

        if(contadorS1==5)
                 {
                 }

        if(contadorS1>=4)
        {
            servo_angulo_acc_base(vec_mpu6050[contadorS1]);
        }
        else
        {
            if(contadorS1==2)
                     {
                servo_angulo_gyr_base(vec_mpu6050[contadorS1]);
                     }
            else{
            servo_angulo_gyr_base(-vec_mpu6050[contadorS1]);
            }
        }

        if(contadorS2>=4)
        {
            servo_angulo_acc_topo(vec_mpu6050[contadorS2]);

        }
        else
        {
            servo_angulo_gyr_topo(vec_mpu6050[contadorS2]);

        }

        if(contadorS1==6)
        {
            vec_mpu6050[1]=0;
            vec_mpu6050[2]=0;
            vec_mpu6050[3]=0;
        }
        if(contadorS2==6)
        {
            vec_mpu6050[1]=0;
            vec_mpu6050[2]=0;
            vec_mpu6050[3]=0;
        }

    }

    P4OUT &= ~BIT7; //Verde apagado
    while (TRUE);
    return 0;
     }


#pragma vector = TIMER0_B0_VECTOR
__interrupt void tb0ccr0(void)
{
    contador_millis+=1;
}



 // Configurar o MPU
 void mpu_config(void)
 {
     mpu_wr(PWR_MGMT_1, 1); //Acodar e Relógio=PLL gx
     delay(100); //Esperar acordar
     mpu_wr(CONFIG, 6); //Taxa = 1 kHz, Banda=5Hz
     mpu_wr(SMPLRT_DIV, 6); //Taxa de amostr. = 100 Hz
     mpu_wr(GYRO_CONFIG, 0x18); // +/- 2000 graus/seg
     mpu_wr(ACCEL_CONFIG, 0x18); // +/- 16g

     mpu_wr(INT_PIN_CFG,0x00);
     mpu_wr(INT_ENABLE,0x01);


     P1DIR &= ~BIT2; //Entrada introut mpu
     P1REN |=  BIT2;
     P1OUT |=  BIT2;

 }

 // Ler sequência de dados do MPU
 void mpu_rd_vet(char reg, char *vt, char qtd)
 {
     char i;
     // Indicar registrador de onde começa a leitura
     UCB0CTL1 |= UCTR | UCTXSTT; //Mestre TX + Gerar START

     while ( (UCB0IFG & UCTXIFG) == 0); //Esperar TXIFG=1

     UCB0TXBUF = reg; //Escrever registrador

     while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT); //STT=0?

     if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG)
     { //NACK?
         P1OUT |= BIT0; //NACK=problema
         while(1); //Travar execução
     }


  // Configurar escravo transmissor
  UCB0CTL1 &= ~UCTR; //Mestre RX
  UCB0CTL1 |= UCTXSTT; //START Repetido

  while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT); //STT=0?

  // Ler a quantidade de dados, menos o último
  for (i=0; i<qtd-1; i++)
  {
  while ((UCB0IFG & UCRXIFG) == 0); //Esperar RX
  vt[i]=UCB0RXBUF; //Ler dado
  }

  // Ler o último dado e gerar STOP
  UCB0CTL1 |= UCTXSTP; //Gerar STOP
  while ((UCB0IFG & UCRXIFG) == 0); //Esperar RX

  vt[i]=UCB0RXBUF; //Ler dado
  while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP); //Esperar STOP
  }


 // Ler um registrador do MPU
char mpu_rd(char reg)
{
   UCB0CTL1 |= UCTR | UCTXSTT; //Mestre TX + Gerar START
   while ( (UCB0IFG & UCTXIFG) == 0); //Esperar TXIFG=1
   UCB0TXBUF = reg; //Escrever registrador
   while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT); //STT=0?
   if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG) //NACK?
   {
       P1OUT |= BIT0; //NACK=problema
       while(1); //Travar execução
   }
   // Configurar escravo transmissor
   UCB0CTL1 &= ~UCTR; //Mestre RX
   UCB0CTL1 |= UCTXSTT; //START Repetido

   while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT); //STT=0?
   UCB0CTL1 |= UCTXSTP; //Gerar STOP

   while ( (UCB0IFG & UCRXIFG) == 0); //Esperar RX

   while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP); //Esperar STOP

   return UCB0RXBUF;
}


  // Escrever num registrador do MPU
void mpu_wr(char reg, char dado)
{
   UCB0CTL1 |= UCTR | UCTXSTT; //Mestre TX + START
   while ( (UCB0IFG & UCTXIFG) == 0); //TXIFG=1?

   UCB0TXBUF = reg; //Escrever dado
   while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT); //Esperar STT=0

   if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG)//NACK?
   {
       P1OUT |= BIT0; //NACK=problema
       while(1); //Travar execução
   }
   while ( (UCB0IFG & UCTXIFG) == 0); //TXIFG=1?

   UCB0TXBUF = dado; //Escrever dado
   while ( (UCB0IFG & UCTXIFG) == 0); //TXIFG=1?

   UCB0CTL1 |= UCTXSTP; //Gerar STOP
   while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP); //Esperar STOP
}


  // Testar o endereço adr para escrita
char i2c_test(char adr)
{
   UCB0I2CSA = adr; //Endereço a ser testado
   UCB0CTL1 |= UCTR; //Mestre TX --> escravo RX
   UCB0CTL1 |= UCTXSTT; //Gerar STASRT
   while ( (UCB0IFG & UCTXIFG) == 0); //TXIFG, já iniciou o START
   UCB0CTL1 |= UCTXSTP; //Gerar STOP
   while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP); //Esperar STOP
    if ((UCB0IFG & UCNACKIFG) == 0) return TRUE; //Chegou ACK
    else return FALSE; //Chegou NACK
}
   // Configurar USCI_B0 como mestre
   // P3.0 = SDA e P3.1 = SCL
void USCI_B0_config(void)
{
    UCB0CTL1 = UCSWRST; //Ressetar USCI_B0
    UCB0CTL0 = UCMST | //Modo Mestre
    UCMODE_3 | //I2C
    UCSYNC; //Síncrono

    //UCB0BRW = BR10K; //10 kbps
    UCB0BRW = BR100K; //100 kbps
    UCB0CTL1 = UCSSEL_3; //SMCLK e UCSWRST=0

    P3SEL |= BIT1 | BIT0; //Funções alternativas
    P3REN |= BIT1 | BIT0; //Pullup
    P3OUT |= BIT1 | BIT0; //Pullup
   }
   // Configurar I/O
void leds_s1_config(void)
{
    P1DIR |= BIT0; P1OUT &= ~BIT0; //Led Vermelho
    P4DIR |= BIT7; P4OUT &= ~BIT7; //Led Verde
    P2DIR &= ~BIT1; //Chave S1
    P2REN |= BIT1; //Chave S1
    P2OUT |= BIT1; //Chave S1



   }
void set_timer_millis(void)
{
        TB0CTL= TBSSEL_2 | MC_1 ;
        TB0CCTL0 |= CCIE ;
        TB0CCR0= 1049;
}


// Gerar atrasos
void delay (int x)
{
    volatile int i;
    for (i=0; i<x; i++);
}

void gpio_config(void){
    P1DIR &= ~BIT1;     //S2
    P1REN |= BIT1;
    P1OUT |= BIT1;
    P1SEL |= BIT1;

    P2DIR &= ~BIT1;     //S1
    P2REN |= BIT1;
    P2OUT |= BIT1;
}
void monitorS1(int *contadorS1,int *S1)
{
    if((P2IN & BIT1)==0)
    {
        if(*S1==0)
        {
            if(*contadorS1==6)
            {
                *contadorS1=0;
                *S1=1;
                delay(2000);
            }
            else
            {
            *contadorS1=*contadorS1+1;
            *S1=1;
            delay(2000);
            }
        }
    }
    else if((P2IN & BIT1)==BIT1)
    {
        if(*S1==1)
        {
            *S1=0;
            delay(2000);
        }
    }

}
void monitorS2(int *contadorS2,int *S2)
{
    if((P1IN & BIT1)==0)
    {
        if(*S2==0)
        {
            if(*contadorS2==6)
            {
                *contadorS2=0;
                *S2=1;
                delay(2000);
            }
            else
            {
            *contadorS2=*contadorS2+1;
            *S2=1;
            delay(2000);
            }
        }
    }
    else if((P1IN & BIT1)==BIT1)
    {
        if(*S2==1)
        {
            *S2=0;
            delay(2000);
        }
    }

}
void pwm_init(void)
{
    P1DIR |= BIT5;
    P1SEL |= BIT5;

    P1DIR |= BIT4;
    P1SEL |= BIT4;

    TA0CTL = TASSEL_2 | MC_1; //Usa ACLK, Modo up
    TA0CCR0=20970;

    TA0CCTL4 = OUTMOD_6 | OUT ;
    TA0CCR4=1600; //

    TA0CCTL3 = OUTMOD_6 | OUT ;
    TA0CCR3=1600; //
}


// essa funcao sera dada a partir de um valor que incrementara a posição angular do servo.
void servo_desloca(int graus_deslocamento)
{
    static unsigned int posicao_passada=600; // 1600
    static unsigned int posicao_corrente=0; // 1600
    const int k = 12;

    posicao_corrente = posicao_passada + k*graus_deslocamento;
    posicao_passada=posicao_corrente;


    TA0CCR4=posicao_corrente; // posição setada para ser 90 graus

}

void servo_angulo_gyr_base(float angulo)
{
    TA0CCR3=(1550 + 12*angulo);
}
void servo_angulo_gyr_topo(float angulo)
{
    TA0CCR4=(2200 + 12*angulo);
}
void servo_angulo(float anguloX, float anguloY)
{

    TA0CCR4=(1550 + 12*anguloX);
    TA0CCR3=(1550 + 12*anguloY);
}

void servo_angulo_acc(float AccX, float AccY)
{
    static float vet_AccX[10]={0,0,0,0,0,0,0,0,0,0};
    static float vet_AccY[10]={0,0,0,0,0,0,0,0,0,0};

   volatile int p;
   volatile float media_ax=0,media_ay=0;
   for(p=9;p>=1;p--)
   {
       vet_AccX[p]=vet_AccX[p-1];
       vet_AccY[p]=vet_AccY[p-1];
       media_ax+=vet_AccX[p];
       media_ay+=vet_AccY[p];
   }
   vet_AccX[0]=AccX;
   vet_AccY[0]=AccY;

   media_ax+=vet_AccX[0];
   media_ay+=vet_AccY[0];
   media_ax/=10;
   media_ay/=10;

    TA0CCR4=(2200 + 12*media_ax*90);
    TA0CCR3=(2200 + 12*media_ay*90);

}

void servo_angulo_acc_base(float Acc)
{
    static float vet_Acc[10]={0,0,0,0,0,0,0,0,0,0};

   volatile int p;
   volatile float media=0;
   for(p=9;p>=1;p--)
   {
       vet_Acc[p]=vet_Acc[p-1];
       media+=vet_Acc[p];
   }
   vet_Acc[0]=Acc;

   media+=vet_Acc[0];
   media/=10;

   TA0CCR3=(2200 + 12*media*90);

}
void servo_angulo_acc_topo(float Acc)
{
    static float vet_Acc[10]={0,0,0,0,0,0,0,0,0,0};

   volatile int p;
   volatile float media=0;
   for(p=9;p>=1;p--)
   {
       vet_Acc[p]=vet_Acc[p-1];
       media+=vet_Acc[p];
   }
   vet_Acc[0]=Acc;

   media+=vet_Acc[0];
   media/=10;

   TA0CCR4=(2200 + 12*media*90);

}

void media_movel_angulo(float* anguloX, float* anguloY)
{
    static float media_x[10]={0,0,0,0,0,0,0,0,0,0};
    static float media_y[10]={0,0,0,0,0,0,0,0,0,0};
    volatile int k_counter=0;
    float VX=0.0;
    float VY=0.0;

    for(k_counter=0;k_counter<10;k_counter++)
    {
        media_x[k_counter+1]=media_x[k_counter];
        media_x[k_counter+1]=media_x[k_counter];
        if(k_counter+1 == 10)
        {
            media_x[0]=*anguloX;
            media_y[0]=*anguloY;
            break;
        }


    }

    for(k_counter=0;k_counter<10;k_counter++)
    {
        VX+=media_x[k_counter];
        VY+=media_y[k_counter];
    }

    *anguloX=VX/10;
    *anguloY=VY/10;


}
