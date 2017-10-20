 /**
  @file Decoder.cpp
  @brief Realiza el trabajo de descomprimir la imagen

  @author Felipe Tambasco, Mauro Barbosa
  @date Feb, 2017

*/

#include "Decoder.h"
#include "ContextRun.h"
#include "math.h"

namespace std {

Decoder::Decoder(CodedImage codedImage) {

	//constructor

	this->file=codedImage.path+codedImage.name+"_decoded_.pgm";

		this->codedImage=codedImage;

		image.heigth=codedImage.heigth;

		image.width=codedImage.width;

		image.setImage();

		Nmax=codedImage.Nmax;


		/**
		 *
		 * CAMBIOS !!
		 */

		this->beta=8;
		this->Lmax=4*beta;

		this->qMax=Lmax-beta-1;
		this->qMax_=Lmax-beta-1;


}

Decoder::~Decoder() {
	// TODO Auto-generated destructor stub
}

void Decoder::decode(){

	/** como puede verse el funcionamiento general del decodificador es bastante simétrico al codificador

	Por una descripción de los métodos en común con la clase Coder, recurrir a las descripciones disponibles en Coder.cpp */

int contadorH=1,contadorW=1,contador=0;
		ofstream salida;
		salida.open(file.c_str(), ios::binary);

		setContextsArray();

		writeHeader(salida);	//escribe encabezado en el archivo de salida

		codedImagePointer=0;

		cout << "// START DECODER" << endl;
		//cout << "Tam: " << codedImage.heigth*codedImage.width << endl;
			while (contadorH<codedImage.heigth+1){
				contadorW=1;
				while (contadorW!=codedImage.width+1){

					//if (contador>22000) debug=true;

/*
							if (contador==1350) {
								cout <<"imagen: ";
								for (int aux_cont=0;aux_cont<2025;aux_cont++)
										cout <<image.image[aux_cont]<<" ";
								cout <<endl;
							}
*/

				pixels pxls = getPixels_(contador);	//trae el vecindario a, b y c de cada pixel a decodificar

				//int p = getP(pxls);	//calcula p

				//if (contador==0)
				if (debug) cout<<contador<<" "  << pxls.a<<" "<< pxls.b<<" "<< pxls.c<<" "<< pxls.d<<endl;
				//cout<<"[" << contador << "] " << pxls.a << " " << pxls.b << " " << pxls.c << " " << pxls.d << endl;

				//cout<<contador<<" "  << pxls.a<<" "<< pxls.b<<" "<< pxls.c<<" "<< pxls.d<<endl;


				grad gradients=setGradients(pxls);	//calcula el vector de gradientes

				//cout<<contador<<" " << gradients.ga<<" "<< gradients.gb<<" "<< gradients.gc<<" "<<endl;

				int signo;

				int contexto = getContext(gradients,signo);	//trae el contexto que corresponde a estte pixel

				//signo=1;

				if (debug) cout<<"signo: "<<signo<<endl;

				if (debug) cout<<contexto<<endl;

				if (contexto==364) {
					racha=true;
					if (debug) cout<<"RACHA!"<<endl;
				}
				else racha=false;

				if (!racha){
				//if (true){

				int predicted = getPredictedValue(pxls);	//calcula el valor predicho

				if (debug) cout<<predicted<<endl;

				predicted=fixPrediction(predicted,signo, contexto);

				if (debug) cout<<predicted<<endl;

				int k= getK(contexto);	//calcula k

				int error_=getError(k,0,0);	//lee el archivo para tener el valor del error codificado


				int error=unRice(error_,get_s(contexto),k);	//deshace el mapeo de rice para recuperar el error real

				error=reduccionDeRango(error,signo,predicted);

				if (debug) cout<<"error= "<<error<<endl;

				if (debug) cout<<"k= "<<k<<endl;

				if (debug) cout<<"error_= "<<error_<<endl;

				//int error_s=error*signo;

				int pixel=predicted+error;	//calcula el pixel como la suma entre el predicho y el error

				updateImage(pixel,contador);	//va formando el array que representa la imagen con cada pixel decodificado

				char pixel_ =pixel+'\0';

				salida.write(&pixel_,1);	//escribe el pixel en el archivo

				updateContexto(contexto,error*signo);	//actualiza A y N del contexto



				}

				else {

				int interruption=0;
				int cantidad_unos=0;

				int largo= getRachaParams(contadorW, interruption,cantidad_unos);

		//		int contexto=(pxls.a==pxls.b);
				int contexto=getContext_(contador, largo);

				Racha racha(largo, interruption, pxls.a,contexto);

				if (debug) cout<<contador<<" "<<largo<<" "<<interruption<<" "<<pxls.a<<" "<<contexto<<endl;

				updateImageRacha(racha, contador, salida);
				if(contador+largo<codedImage.heigth*codedImage.width) updateImageInterruption(racha, contador,contador+largo, salida,cantidad_unos);

				racha.updateContexto();

				contadorW=contadorW+largo;contador=contador+largo; //está bien?

				if (racha.interruption)	{
					contadorW--;
					contador--;
				}

				if (debug) cout<<contadorW<<" "<<contadorH<<endl;

				if (debug) cout<<endl;
				}contador++;contadorW++;

				}contadorH++;


			}
		salida.close();
}float Decoder::get_s(int contexto){


	return float(float(contexts[contexto].B)/float(contexts[contexto].N)); //es N o N_?
}

int Decoder::reduccionDeRango(int error, int signo,int predicted){

	//IF(J3<0,J3+$A$3,IF(J3>$A$3,J3-$A$3,J3))
	error=error*signo;

	if ((error+predicted)<0){

		error=error+256;
	}else if ((error+predicted)>255){

		error=error-256;
	}


	if (debug) cout<<"return error"<<endl;

	return error;


	/*
	 int cociente=0;

	 int mod=error%128;
	 if (error<0)
		 cociente=error/129;
	 else
		 cociente=error/128;


	 if (cociente*cociente==1){

		 if (cociente==1){

			 error=-255+error-1;

		 }else{
			 error=-error;
			 error=255-error+1;

		 }

	 }
*/
	return error;
}

int Decoder::getKPrime(Racha &r){
	int T_racha, K_racha;

	//cout<<">> RACHA CNTX="<<r.contexto<<" A="<<cntx[r.contexto].A_racha<<" N="<<cntx[r.contexto].N_racha<<" Nn="<<cntx[r.contexto].Nn_racha;

	// Calculo la variable T para determinar k de Golomb.
	T_racha=((r.contexto==1) ? cntx[r.contexto].A_racha : cntx[r.contexto].A_racha + cntx[r.contexto].N_racha/2);   
	
	for(K_racha=0; (cntx[r.contexto].N_racha*pow(2,K_racha))<T_racha; K_racha++);

	//cout<<" T="<<T_racha<<" K="<<K_racha<<endl;
	
	return K_racha;
}
int Decoder::unrice_rachas(int error,int contexto, int k){

	int map=0;

	int retorno=error+contexto+1>> 1;

	if ((error+contexto+1)%2==0) map=1;

	if ((k==0)and(map==1)and(2*cntx[contexto].Nn_racha<cntx[contexto].N_racha)){

	}else if ((map==1)and(2*cntx[contexto].Nn_racha>=cntx[contexto].N_racha)){

		retorno=-retorno;
	}else if ((map==1)and(k!=0)){

		retorno=-retorno;
	}else{

		if ((k!=0)or(2*cntx[contexto].Nn_racha>=cntx[contexto].N_racha)){


		}else retorno=-retorno;


	}

	 return retorno;

}
void Decoder::updateImageInterruption(Racha &racha, int contador,int prox_, ofstream &salida,int cantidad_unos){

	if (!racha.interruption){

	int kPrime=getKPrime(racha);
	int error_=getError(kPrime,1,cantidad_unos);
	//int error = unRice(error_,0,1);

	int error = unrice_rachas(error_,racha.contexto,kPrime);

	error=reduccionDeRango(error,1,getPixels_(prox_).b);
	int errorEstadisticos=clipErrorEstadisticos(error);

	if (debug) cout<<"error: "<<error_<<" "<<error<<endl;

	image.image[contador+racha.largo]=getPixels_(prox_).b+error;

	char pixel_ =getPixels_(prox_).b+error+'\0';

	salida.write(&pixel_,1);	//escribe el pixel en el archivo
	updateContexto_(racha.contexto, errorEstadisticos);

	if (debug) cout<<"actual: "<<image.image[contador+racha.largo]<<endl;

	}

}

void Decoder::updateImageRacha(Racha &racha, int contador, ofstream &salida){

	for (int k=0;k<racha.largo;k++){

		image.image[contador+k]=racha.pixel;

		char pixel_ =racha.pixel+'\0';

		salida.write(&pixel_,1);	//escribe el pixel en el archivo
	}


}
int Decoder::getRachaParams(int contadorW, int &interruption_, int &cantidad_unos){

	kr=0;

	int bit=1;

	int largo=0, interruption;


	bool finDeFila=false;

	cantidad_unos=0;

	while ((!finDeFila)&&(bit=getBit())){

//		if ((bit=getBit())==0) break;

		cantidad_unos++;

		m_r=pow(2,J[kr]);
		kr++;

		largo=largo+m_r;

		if (debug) cout<<"kr= "<<kr<<endl;

		finDeFila=(largo+contadorW-1>=image.width); //mayor o mayor o igual?

		//if (debug) cout<<"largo+contadorW= "<<largo+contadorW<<endl;

	}//cout<<"fileToBitsPointer= "<<fileToBitsPointer<<endl;

	interruption=bit;

	if ((largo+contadorW-1>image.width)) cantidad_unos--;

	kr--;





	if (bit)	largo=image.width-contadorW+1;
	else {

		int pot;

		if (debug) cout<<"ok"<<kr;

		for (int j=kr;j>=0;j--){

			pot=(pow(2,j))*getBit();

			largo=largo+pot;

		}

	}if (debug) cout<<endl;

	kr++;

	cantidad_unos=J[kr]+1;
	if (debug) cout<<"J[kr]+1= "<<cantidad_unos<<endl;

	interruption_=interruption;

	return largo;
}

void Decoder::updateImage(int pixel, int contador){

	/** Agrega el pixel decodificado al array que representa la imagen */

	image.image[contador]=pixel;

}
int Decoder::unRice(int error,float s, int k){

	/** Inverso de mapeo de rice */

	if ((k<=0)and(s<=-0.5)){

			if (error%2==0)	{
				int aux=error/2;
				return -aux-1;
			}
			else{

				int aux=((error+1)/(-2));
				return -aux-1;
			}
		}

	else{

	if (error%2==0)	return error/2;
	else	return ((error+1)/(-2));
	}
}

void Decoder::completaArray(){

	/** Cuando se lee el último bit disponible del array, este método vuelve a completarlo. */

	char temp;

	int contador=0;

	while ((contador<100)&&((codedImagePointer<(codedImage.heigth*codedImage.width)))){ //hasta leer 100 bytes o que se termine la imagen

	temp=codedImage.image[codedImagePointer];
	codedImagePointer++;

	std::bitset<8> temp_b(temp);

				for(int j=0;j<8;j++){

				fileToBits[contador*8+j]=temp_b[7-j];

				}//for j

				contador++;
	}

}

int Decoder::getBit(){

	/** Esta función devuelve el próximo bit de fileToBits.
	Cuando llega al último elemento del array, vuelve a llenar el array con los valores de la imagen
	y empieza desde el lugar 0 */

	if (fileToBitsPointer==0){

		completaArray();

	}int retorno = fileToBits[fileToBitsPointer];
	if (debug) cout<<fileToBits[fileToBitsPointer];
	fileToBitsPointer=((fileToBitsPointer+1)%800);//actualiza el puntero al array de manera circular

	//if (debug) cout<<retorno;
	return retorno;

}

int Decoder::getError_(int k){

	/** Devuelve como entero el error codificado */

	int error=0;
	int potencia=pow(2,k);

	int bit=0;

	/* Convierte los siguientes k bits de fileToBits en un entero,
	que corresponden a la parte binaria del error */
	for (int j=0;j<k;j++){

		bit=getBit();

		potencia=potencia/2;
			error=error+bit*potencia;

	}


		return error;
}

int Decoder::getError(int k, int racha, int ajuste){

	/** Devuelve como entero el error codificado */

	int qMax;

	if (racha) {

		qMax=this->qMax_;
		qMax=qMax-ajuste;

	}
	else {

		qMax=this->qMax;
	}

	int error=0;
	int potencia=floor(pow(2,k));
	//if (potencia==0) potencia=1;

	int bit=0;

	int contador=0;

	/* Obtiene la cantidad de ceros que le siguen antes del primer uno,
		es la codificación unaria del cociente entre el error y 2^k */
	while ((contador!=qMax)&&getBit()!=1){
	//while (getBit()!=1){
			contador++;

		}
	if (debug) cout<<"contador=qMax: "<<(contador==qMax)<<endl;

	if (contador!=qMax){

	/* Convierte los siguientes k bits de fileToBits en un entero,
	que corresponden a la parte binaria del error */
	for (int j=0;j<k;j++){

		bit=getBit();

		potencia=potencia/2;
			error=error+bit*potencia;

	}
	}
	else{

		potencia=pow(2,8);

		for (int j=0;j<8;j++){

				bit=getBit();

				potencia=potencia/2;
					error=error+bit*potencia;

			}


	}

	int pot_aux=1;

	if (k>=0) pot_aux=pow(2,k); //cuando k negativo pow(2,k) es 0, y necesitamos que sea 1


		/* Sumando los dos valores decodificados (cociente y resto entre 2^k) resulta el mapeo de rice
		del error codificado */
		if (contador!=qMax) error=error+contador*pot_aux;

		if (debug) cout<<endl;



		return error;
}

void Decoder::writeHeader(ofstream &salida){

	writeMagic(salida);
	writeWidth(salida);
	writeHeigth(salida);
	writeWhite(salida);

}

void Decoder::writeMagic(ofstream &salida){
	char temp='P';
	salida.write(&temp,1);

	temp='5';
	salida.write(&temp,1);

	temp='\n';
	salida.write(&temp,1);
}

void Decoder::writeWidth(ofstream &salida){

	int ancho =codedImage.width;

	double aux=(double)ancho;

	int potencia=1;

	while(aux>1){

		aux=aux/10;
		potencia=potencia*10;
	}

	char temp_;

	int temp=0;

	while(potencia>1){


		aux=aux-(double)temp;
	aux=aux*double(10);

	if (double(ceil(aux))-(double)aux<(double)0.00001)
						temp=ceil(aux);			//parche artesanal, algunos números uno los ve como
												//cierto valor, pero al tomar el floor te da el entero
												//anterior, supongo que si bien uno lo ve como el número n
												//para la máquina es (n-1),9999999999
	else temp=floor(aux);

	temp_=temp+'0';

	salida.write(&temp_,1);

	potencia=potencia/10;
	}

	temp_=' ';

	salida.write(&temp_,1);
}
void Decoder::writeHeigth(ofstream &salida){
	int alto =codedImage.heigth;

	double aux=(double)alto;

	int potencia=1;

	while(aux>1){

		aux=aux/10;
		potencia=potencia*10;
	}

	char temp_;

	int temp=0;

	while(potencia>1){

		aux=aux-(double)temp;

	aux=aux*double(10);

	if (double(ceil(aux))-(double)aux<(double)0.00001)
						temp=ceil(aux);			//parche artesanal, algunos números uno los ve como
												//cierto valor, pero al tomar el floor te da el entero
												//anterior, suponemos que si bien uno lo ve como el número n
												//para la máquina es (n-1),9999999999
	else temp=floor(aux);

	temp_=temp+'0';

	salida.write(&temp_,1);

	potencia=potencia/10;
	}

	temp_='\n';

	salida.write(&temp_,1);
}
void Decoder::writeWhite(ofstream &salida){

	int blanco =codedImage.white;

	double aux=(double)blanco;

	int potencia=1;

	while(aux>1){

		aux=aux/10;
		potencia=potencia*10;
	}

	char temp_;

	int temp=0;

	while(potencia>1){

		aux=aux-(double)temp;
		aux=aux*double(10);

		if (double(ceil(aux))-(double)aux<(double)0.00001)
							temp=ceil(aux);			//parche artesanal, algunos números uno los ve como
													//cierto valor, pero al tomar el floor te da el entero
													//anterior, suuponemos que si bien uno lo ve como el número n
													//para la máquina es (n-1),9999999999
		else temp=floor(aux);

	temp_=temp+'0';

	salida.write(&temp_,1);

	potencia=potencia/10;
	}

	temp_='\n';

	salida.write(&temp_,1);

}

int Decoder::fixPrediction(int predicted,int signo, int contexto){

	predicted=predicted+(contexts[contexto].C*signo);

	if (predicted< 0) predicted=0;
	if (predicted >255) predicted=255;

	return predicted;
}

void Decoder::updateContexto(int contexto, int error){

	/** Actualiza los datos N y A del contexto */

		/** Actualiza B y C */

		if (contexts[contexto].N==Nmax){

			/* si el valor de N para ese contexto es igual a Nmax divide N y A entre 2 */
			contexts[contexto].N=contexts[contexto].N/2;
			contexts[contexto].N_=contexts[contexto].N_/2;
			contexts[contexto].A=floor((double)contexts[contexto].A/(double)2);

			contexts[contexto].B=floor((double)contexts[contexto].B/(double)2);

		}
		/* Actualiza A sumándole el valor absoluto de este error */
		contexts[contexto].B=contexts[contexto].B+error;

		contexts[contexto].A=contexts[contexto].A+abs(error);

		contexts[contexto].N++;	//actualiza N

		if (error<0) contexts[contexto].N_++;

		if (contexts[contexto].B<=-contexts[contexto].N){

			contexts[contexto].B=contexts[contexto].B+contexts[contexto].N;

			if (contexts[contexto].C>-128) contexts[contexto].C=contexts[contexto].C-1;
			if (contexts[contexto].B<=-contexts[contexto].N) contexts[contexto].B=-contexts[contexto].N+1;



		}

		else if (contexts[contexto].B>0){


			contexts[contexto].B=contexts[contexto].B-contexts[contexto].N;

					if (contexts[contexto].C<127) contexts[contexto].C=contexts[contexto].C+1;
					if (contexts[contexto].B>0) contexts[contexto].B=0;



		}

	/*	if (!(contexts[contexto].C==0)){

			cout<<"B: "<<contexts[contexto].B<<endl;
			cout<<"C: "<<contexts[contexto].C<<endl;

		}
		*/
}


int Decoder::getK(int contexto){

	double AdivN_=(double)contexts[contexto].A/(double)contexts[contexto].N;

	return round(log2(AdivN_));
}

int Decoder::getPredictedValue(pixels pxls){

	if ((pxls.c>=pxls.a)&&(pxls.c>=pxls.b)){

		if (pxls.a>pxls.b)
				return pxls.b;
		else return pxls.a;

	}else if ((pxls.c<=pxls.a)&&(pxls.c<=pxls.b)){

		if (pxls.a>pxls.b)
				return pxls.a;
		else return pxls.b;

	}else return (pxls.a+pxls.b-pxls.c);



}

int Decoder::getContext(grad gradients, int &signo){

	signo=1;

	int contga, contgb,contgc;

		if (gradients.ga<-21) contga=0;
		else if (gradients.ga<-7) contga=1;
		else if (gradients.ga<-3) contga=2;
		else if (gradients.ga<0) contga=3;
		else if (gradients.ga==0) contga=4;
		else if (gradients.ga<=3) contga=5;
		else if (gradients.ga<=7) contga=6;
		else if (gradients.ga<=21) contga=7;
		else contga=8;

		if (gradients.gb<-21) contgb=0;
			else if (gradients.gb<-7) contgb=1;
			else if (gradients.gb<-3) contgb=2;
			else if (gradients.gb<0) contgb=3;
			else if (gradients.gb==0) contgb=4;
			else if (gradients.gb<=3) contgb=5;
			else if (gradients.gb<=7) contgb=6;
			else if (gradients.gb<=21) contgb=7;
			else contgb=8;

		if (gradients.gc<-21) contgc=0;
				else if (gradients.gc<-7) contgc=1;
				else if (gradients.gc<-3) contgc=2;
				else if (gradients.gc<0) contgc=3;
				else if (gradients.gc==0) contgc=4;
				else if (gradients.gc<=3) contgc=5;
				else if (gradients.gc<=7) contgc=6;
				else if (gradients.gc<=21) contgc=7;
				else contgc=8;


	if(contga<4){
		contga=8-contga;
		contgb=8-contgb;
		contgc=8-contgc;

		signo=-1;

	}

			//mapeo elegido para representar los contextos
			return (9*9*contga)+(9*contgb)+(contgc);
}

void Decoder::setContextsArray(){

	/** Forma el array con todos los contextos posibles */

		int indice=0;

		for (int k=-4;k<5;k++){

			for (int j=-4;j<5;j++){

				for (int i=-4;i<5;i++){

						Context contexto(k,j,i);
						contexts[indice]=contexto;
						indice++;

				}

			}

		}

}

Decoder::grad Decoder::setGradients(pixels pxls){

	/** Dado p y los píxeles a, b, c y d de la vecindad,
	forma el vector de gradientes */

	grad gradients={pxls.d-pxls.b,pxls.b-pxls.c,pxls.c-pxls.a};

	return gradients;
}

int Decoder::getP(pixels pxls){

	return floor((double)(2*pxls.a+2*pxls.b+2*pxls.c+3)/(double)6);

}Decoder::pixels Decoder::getPixels_(int current){

	/** Devuelve los píxeles de la vecindad: a, b y c */

	int a=-1;
	int b=-1;
	int c=-1;
	int d=-1;

	if (current==0){

		//primer píxel

		a=0;
		b=0;
		c=0;
		d=0;

	}	else if ((current%image.width)==0){

		/* columna izquierda */

		a=image.image[current-image.width];
		c=getPixels_(current-image.width).a;


	}

	if (current<image.width){

			//primer fila

			if (b==-1) b=0;
			if (c==-1) c=0;
			if (d==-1) d=0;
		}




	else if ((current%image.width)==image.width-1){

		/* columna derecha */

		d=image.image[current-image.width];


	}


	/* Para cada a, b,c y d, si no se cumple una condición de borde, y por lo tanto no hubo asignación en los if que preceden,
	se traen los valores de a, b,c y d de la imagen */
	if (a==-1) a=image.image[current-1];
	if (b==-1) b=image.image[current-image.width];
	if (c==-1) c=image.image[current-image.width-1];
	if (d==-1) d=image.image[current-image.width+1];

	pixels pxls={a,b,c,d};

		return pxls;
}

Decoder::pixels Decoder::getPixels(int current){

	/** Devuelve los píxeles de la vecindad: a, b y c */

		int a=-1;
		int b=-1;
		int c=-1;
		int d=-1;

		if ((current%codedImage.width)==0){

			/* Si estoy parado en un borde izquierdo, el valor de a y c tienen que ser "128",
			o la mitad del valor de blanco de la imagen */
			a=ceil((double)codedImage.white/(double)2);
			c=ceil((double)codedImage.white/(double)2);


		}

		if ((current%image.width)==codedImage.width-1){

			/* Si estoy parado en un borde derecho, el valor de d tiene que ser "128",
			o la mitad del valor de blanco de la imagen */
			d=ceil((double)codedImage.white/(double)2);


		}

		if (current<codedImage.width){

			/* Si estoy en la primer fila, b y c deben ser "128"
			o la mitad del valor de blanco de la imagen */
			if (b==-1) b=ceil((double)codedImage.white/(double)2);
			if (c==-1) c=ceil((double)codedImage.white/(double)2);
			if (d==-1) d=ceil((double)codedImage.white/(double)2);
		}

		/* Para cada a, b,c y d, si no se cumple una condición de borde, y por lo tanto no hubo asignación en los if que preceden,
		se traen los valores de a, b,c y d de la imagen */
		if (a==-1) a=image.image[current-1];
		if (b==-1) b=image.image[current-codedImage.width];
		if (c==-1) c=image.image[current-codedImage.width-1];
		if (d==-1) d=image.image[current-codedImage.width+1];

		pixels pxls={a,b,c,d};

			return pxls;
}

int Decoder::getContext_(int pos, int lar){
	return (getPixels_(pos).a==getPixels_(pos+lar).b);
}

void Decoder::updateContexto_(int c, int err){

	if(cntx[c].N_racha>RESET) {
		cntx[c].reset();
		//cout << "RESET" << endl;
	}

	cntx[c].updateA(err);
	cntx[c].updateNn(err);

	cntx[c].updateN();
}

int Decoder::clipErrorEstadisticos(int error){
	int errorEstadisticos=error;
	if(error>128) errorEstadisticos-=256;
	else if(error<-128) errorEstadisticos+=256;
	
	return errorEstadisticos;
}

} /* namespace std */