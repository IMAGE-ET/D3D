 /**
  @file Decoder.cpp
  @brief Realiza el trabajo de descomprimir la imagen

  @author Felipe Tambasco, Mauro Barbosa
  @date Feb, 2017

*/

#include "Decoder.h"
#include "ContextRun.h"
#include "math.h"
#include "Writer.h"
#include "Reader.h"
#include <sstream>

namespace std {
int Decoder::numberImgPath=0;

Decoder::~Decoder() {}

Decoder::Decoder(CodedImage &ci, bool vector) {
	this->codedImage=ci;

	string tipo = (vector ? "_vector_" : "_decoded_");	
	this->file = ci.path + ci.name + tipo;
		
	ancho  = (vector ? ci.v_width  : ci.width);
	alto   = (vector ? ci.v_heigth : ci.heigth);
	blanco = (vector ? ci.v_white  : ci.white);
	Nmax   = ci.Nmax;	

	if (2>ceil(log2(blanco+1))) this->beta=2;
		else this->beta=ceil(log2(blanco+1));

	if (2>ceil(log2(blanco+1))) {
		this->Lmax=2*(2+8);
	} else {
		if (8>ceil(log2(blanco+1))) {
			this->Lmax=2*(ceil(log2(blanco+1))+8);
		}
		else{
			this->Lmax=2*(ceil(log2(blanco+1))+ceil(log2(blanco+1)));
		}
	}

	this->qMax=Lmax-beta-1;
	this->qMax_=Lmax-beta-1;
	this->range=blanco+1;
	this->cantidad_imagenes=(vector ? 2 : ci.cantidad_imagenes);
	this->activarCompMov=ci.activarCompMov;
	this->images = new Image[this->cantidad_imagenes];
	this->bsize = ci.bsize;

//	this->nBits = ceil(log2(blanco+1));
	this->nBits = ((blanco <= 0xFF) ? 8 : 16);
}

void Decoder::setParams(int Nmax, int reset, int t1,int t2,int t3){

	this->Nmax=Nmax;
	this->reset=reset;
	this->t1=t1;
	this->t2=t2;
	this->t3=t3;

}

void Decoder::setContextsArray2D(){
	/** Forma el array con todos los contextos posibles */

	int indice=0;

	for (int k=-4;k<5;k++){
		for (int j=-4;j<5;j++){
			for (int i=-4;i<5;i++){

						//cout<<"blanco: "<<blanco<<endl;
						Context contexto(k,j,i,blanco);
						contexts[indice]=contexto;
						indice++;

			}
		}
	}
}

Decoder::pixels2D Decoder::getPixels2D(int current,Image &image){

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

 	}	else if ((current%ancho)==0){

 		/* columna izquierda */

 		a=image.image[current-ancho];
 		c=getPixels2D(current-ancho,image).a;


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

 	pixels2D pxls={a,b,c,d};

 		return pxls;
 }
Decoder::grad Decoder::getGradients2D(pixels2D pxls){

	/** Dado p y los píxeles a, b, c y d de la vecindad,
	forma el vector de gradientes */

	grad gradients={pxls.d-pxls.b,pxls.b-pxls.c,pxls.c-pxls.a};

	return gradients;

}

int Decoder::getContext2D(grad gradients, int &signo){

 	/** Determina el contexto
 	Todos los contextos posibles se organizan en un array, donde cada elemento del array representa un contexto,
 	es posible definir un mapeo entre el espacio de todos los contextos posibles y los enteros,
 	para que dado un contexto haya una relación biunívoca con un elemento del array */

 	signo=1;

 	int contga, contgb,contgc;

 	if (gradients.ga<=-21) contga=0;
 	else if (gradients.ga<=-7) contga=1;
 	else if (gradients.ga<=-3) contga=2;
 	else if (gradients.ga<0) contga=3;
 	else if (gradients.ga==0) contga=4;
 	else if (gradients.ga<3) contga=5;
 	else if (gradients.ga<7) contga=6;
 	else if (gradients.ga<21) contga=7;
 	else contga=8;

 	if (gradients.gb<=-21) contgb=0;
 		else if (gradients.gb<=-7) contgb=1;
 		else if (gradients.gb<=-3) contgb=2;
 		else if (gradients.gb<0) contgb=3;
 		else if (gradients.gb==0) contgb=4;
 		else if (gradients.gb<3) contgb=5;
 		else if (gradients.gb<7) contgb=6;
 		else if (gradients.gb<21) contgb=7;
 		else contgb=8;

 	if (gradients.gc<=-21) contgc=0;
 		else if (gradients.gc<=-7) contgc=1;
 		else if (gradients.gc<=-3) contgc=2;
 		else if (gradients.gc<0) contgc=3;
 		else if (gradients.gc==0) contgc=4;
 		else if (gradients.gc<3) contgc=5;
 		else if (gradients.gc<7) contgc=6;
 		else if (gradients.gc<21) contgc=7;
 		else contgc=8;

 		if(contga<4){
 			contga=8-contga;
 			contgb=8-contgb;
 			contgc=8-contgc;

 			signo=-1;
 		}else if((contga==4)and(contgb<4)) {
 			contga=8-contga;
 			contgb=8-contgb;
 			contgc=8-contgc;

 			signo=-1;
 		}
 		else if((contga==4)and(contgb==4)and(contgc<4)) {
 					contga=8-contga;
 					contgb=8-contgb;
 					contgc=8-contgc;

 					signo=-1;
 				}

 	//mapeo elegido para representar los contextos
 	return (9*9*contga)+(9*contgb)+(contgc);
 }

int Decoder::getPredictedValue2D(pixels2D pxls){

	/** Calcula el valor predicho según expresión de las diapositivas del curso */

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

void Decoder::setDebug(bool deb){

	this->nuevo_debug=deb;
}

void Decoder::decode2D(Reader &reader, int imgActual){
	cout << "// START DECODER" << endl;
	//cout << "decode(): Imagen/stack de " << nBits << " bits." << endl;

	numberImgPath++; // para diferenciar las imagenes

	if(primeraImagen){
		setContextsArray2D();
	}



//for (int imagen=imgActual; imagen < imgActual + 1; imagen++){


//	string nombre = file + "_" + str_(numberImgPath) + "_" + str_(imgActual);

	string tipo="imagen";

	int largoNombre = file.length() + tipo.length() + 16;
	char nombre[largoNombre];

	sprintf(nombre, "%sD3D_%s_%03d_%03d.pgm", file.c_str(), tipo.c_str(), numberImgPath, imgActual);

	cout << "decode(): Path: " << nombre << endl;
	Writer* writer = new Writer();
	writer->open(nombre);

	writeHeader(*writer);	//escribe encabezado en el archivo de salida

	/*
	Image image(alto,ancho);
	image.white=blanco;
	*/

	Image image(alto, ancho, blanco);

	int y=0;
	int x=0;

	if (!primeraImagen) nuevo_debug=false;

	for(y=0;y<alto;y++){
		for(x=0;x<ancho;x++){
			int signo;
			bool esRacha;



			pixels2D pxls = getPixels2D(y*ancho+x,image);
			grad gradients=getGradients2D(pxls); //calcula los gradientes
			int contexto = getContext2D(gradients, signo);

			esRacha=(contexto==364);

			if (nuevo_debug){

				cout<<endl;
				cout<<endl;
				cout<<"x: "<<x<<" y: "<<y<<endl;
				cout<<"pixels: "<<pxls.a<<" "<<pxls.b<<" "<<pxls.c<<" "<<pxls.d<<endl;
				cout<<"contexto: "<<contexto<<endl;
			}

			if (!esRacha){
				int predicted = getPredictedValue2D(pxls);	//calcula el valor pixel predicho
				predicted     = fixPrediction(predicted,signo, contexto);

				int k      = getK(contexto);	        //calcula k
				int error_ = getError(reader,k,0,0);	//lee el archivo para tener el valor del error codificado
				int error  = unRice(error_,get_s(contexto),k);	//deshace el mapeo de rice para recuperar el error real
				error      = reduccionDeRango(error,signo,predicted);

				int pixel = predicted + error;      //calcula el pixel como la suma entre el predicho y el error
				updateImage(pixel, x, y, image);    //va formando el array que representa la imagen con cada pixel decodificado

				if (nuevo_debug){

					cout<<endl;
					cout<<"fix_predicted: "<<predicted<<endl;
					cout<<"k: "<<k<<endl;
					cout<<"error_: "<<error_<<endl;
					cout<<"error: "<<error<<endl;
					cout<<"pixel: "<<pixel<<endl;
				}

				/*
				char pixel_ =pixel+'\0';
				writer->writeChar(pixel_);	//escribe el pixel en el archivo
				*/

				writer->write(pixel, nBits);

				updateContexto(contexto,error*signo);	//actualiza A y N del contexto
			} else {
				int interruption=0;
				int cantidad_unos=0;
				int largo=getRachaParams2(reader, x,interruption,cantidad_unos);
				int contexto=getContext_(x,y, largo,image);
				Racha racha(largo, interruption, pxls.a,contexto);
				updateImageRacha(racha, x,y, *writer,image);
				if(x + y*ancho + largo < ancho*alto) updateImageInterruption(reader, racha, x,y, x+largo, *writer, cantidad_unos, image);
				x=x+largo;
				if (racha.interruption)	{
					x--;
				}

				if (nuevo_debug){

					cout<<endl;
					cout<<"contexto: "<<contexto<<endl;
					cout<<"racha: "<<largo<<" "<<interruption<<endl;
				}

			}
		}
	}

		if(primeraImagen) primeraImagen=false;
//	}
		writer->close();
	cout << "// END DECODER." << endl;
}


void Decoder::decode(Reader &reader, bool vector, Image &previa, int imgActual){
	cout << "// START DECODER" << endl;
	if(!vector) cout << "decode(): Imagen/stack de " << nBits << " bits." << endl;
	
	numberImgPath++; // para diferenciar las imagenes

	if(primeraImagen){
		setContextsArray();
		prev=setInitialImage();
	} else {
		prev = previa;
	}
	
	if (vector) cout << "decode(): Modo VECTORES" << endl; else cout << "decode(): Modo IMAGEN" << endl;
		
//for (int imagen=imgActual; imagen < imgActual + 1; imagen++){

	if (vector) cout << "decode(): imagen vector: " << imgActual << endl; else cout << "decode(): imagen deco: " << imgActual << endl;

//	string nombre = file + "_" + str_(numberImgPath) + "_" + str_(imgActual);

	string tipo;

	if(!vector                    ) tipo = "imagen";
	if( vector && (imgActual == 0)) tipo = "horizontal";
	if( vector && (imgActual == 1)) tipo = "vertical";

	int largoNombre = file.length() + tipo.length() + 16;
	char nombre[largoNombre];

	sprintf(nombre, "%sD3D_%s_%03d_%03d.pgm", file.c_str(), tipo.c_str(), numberImgPath, imgActual);
		
	cout << "decode(): Path: " << nombre << endl;
	Writer* writer = new Writer();
	writer->open(nombre);

	writeHeader(*writer);	//escribe encabezado en el archivo de salida

	/*
	Image image(alto,ancho);
	image.white=blanco;
	*/

	Image image(alto, ancho, blanco);
	
	int y=0;
	int x=0;
	int x_prev,y_prev;
	for(y=0;y<alto;y++){
		for(x=0;x<ancho;x++){
			int signo;
			bool esRacha;
			getProxImageAnterior(x,y,x_prev,y_prev,vector);
			pixels3D pxls = getPixels3D(x_prev,y_prev,x,y,image);
			grad gradients = getGradients3D(1,pxls);
			int contexto = getContext(getGradients3D(0,pxls), getGradients3D(4,pxls), signo, esRacha);

			if (!esRacha){
				int predicted = getPredictedValue(selectMED(gradients),pxls);	//calcula el valor pixel predicho
				predicted     = fixPrediction(predicted,signo, contexto);
				
				int k      = getK(contexto);	        //calcula k
				int error_ = getError(reader,k,0,0);	//lee el archivo para tener el valor del error codificado
				int error  = unRice(error_,get_s(contexto),k);	//deshace el mapeo de rice para recuperar el error real
				error      = reduccionDeRango(error,signo,predicted);
				
				int pixel = predicted + error;      //calcula el pixel como la suma entre el predicho y el error
				updateImage(pixel, x, y, image);    //va formando el array que representa la imagen con cada pixel decodificado
				
				/*
				char pixel_ =pixel+'\0';
				writer->writeChar(pixel_);	//escribe el pixel en el archivo
				*/
				
				writer->write(pixel, nBits);
				
				updateContexto(contexto,error*signo);	//actualiza A y N del contexto
			} else {
				int interruption=0;
				int cantidad_unos=0;
				int largo=getRachaParams2(reader, x,interruption,cantidad_unos);
				int contexto=getContext_(x,y, largo,image);
				Racha racha(largo, interruption, pxls.a,contexto);
				updateImageRacha(racha, x,y, *writer,image);				
				updateImageInterruption(reader, racha, x,y, x+largo, *writer, cantidad_unos, image);
				x=x+largo;
				if (racha.interruption)	{
					x--;
				}
			}
		}
	}
		previa=image;

		if(vector && imgActual == 0){
			for(int i=0; i<ancho*alto; i++) {
				codedImage.vector_ancho[i] = previa.image[i];
			}
		}
		if(vector && imgActual == 1){
			for(int i=0; i<ancho*alto; i++) {
				codedImage.vector_alto[i] = previa.image[i];
			}
		}
		if(primeraImagen) primeraImagen=false;
//	}
		writer->close();
	cout << "// END DECODER." << endl;
}

string Decoder::str_(int n){
	stringstream ss1;
	ss1 << n;
	string n_ = ss1.str();

	return n_;
}

void Decoder::getProxImageAnterior(int x, int y, int &x_prev, int &y_prev, bool vector){
	x_prev = x;
	y_prev = y;

	if (activarCompMov && !vector){
		int bloqueV = y / bsize;
	 	int bloqueH = x / bsize;
		x_prev = x + codedImage.getPixelAncho(bloqueH,bloqueV)-codedImage.v_white/2;
		y_prev = y + codedImage.getPixelAlto(bloqueH,bloqueV)-codedImage.v_white/2;
	}
}

Decoder::pixels3D Decoder::getPixels3D(int x_prev,int y_prev,int x,int y,Image &image2){
	/** Devuelve los píxeles de la vecindad: a, b, c, d, a_, b_, c_, d_, e_, f_ y g_ */
	/**  arreglar criterio para los píxeles que caen fuera de la imagen*/

	Image image=prev;

	int a=-1;
	int b=-1;
	int c=-1;
	int d=-1;
	int a_=-1;
	int b_=-1;
	int c_=-1;
	int d_=-1;
	int e_=-1;
	int f_=-1;
	int g_=-1;

	if (x==0){
		/* Si estoy parado en un borde izquierdo, el valor de a y c tienen que ser "128",
		o la mitad del valor de blanco de la imagen */
		a = 1 + (blanco >> 1);
		c = 1 + (blanco >> 1);
	}

	if (x==ancho-1){
		/* Si estoy parado en un borde derecho, el valor de d tiene que ser "128",
		o la mitad del valor de blanco de la imagen */

		d = 1 + (blanco >> 1);
	}

	if (y==0){
		/* Si estoy en la primer fila, b y c deben ser "128"
		o la mitad del valor de blanco de la imagen */

		if(b == -1) b = 1 + (blanco >> 1);
		if(c == -1) c = 1 + (blanco >> 1);
		if(d == -1) d = 1 + (blanco >> 1);
	}

	if (x_prev==0){
		/* Si estoy parado en un borde izquierdo, el valor de a y c tienen que ser "128",
		o la mitad del valor de blanco de la imagen */

		a_ = 1 + (blanco >> 1);
		c_ = 1 + (blanco >> 1);
	}

	if (x_prev==ancho-1){
		/* Si estoy parado en un borde derecho, el valor de d tiene que ser "128",
		o la mitad del valor de blanco de la imagen */

		d_ = 1 + (blanco >> 1);
		f_ = 1 + (blanco >> 1);
	}

	if (y_prev==0){
		/* Si estoy en la primer fila, b y c deben ser "128"
		o la mitad del valor de blanco de la imagen */

			if(b_ == -1) b_ = 1 + (blanco >> 1);
			if(c_ == -1) c_ = 1 + (blanco >> 1);
			if(d_ == -1) d_ = 1 + (blanco >> 1);
	}

	if (y_prev>alto-2){
		/* Si estoy en la última o penúltima fila, g debe ser "128"
		o la mitad del valor de blanco de la imagen */

		g_ = 1 + (blanco >> 1);
	}

	/* Para cada a, b,c y d, si no se cumple una condición de borde, y por lo tanto no hubo asignación en los if que preceden,
	se traen los valores de a, b,c y d de la imagen */

	if (a==-1)  a=image2.getPixel(x-1,y);
	if (b==-1)  b=image2.getPixel(x,y-1);
	if (c==-1)  c=image2.getPixel(x-1,y-1);
	if (d==-1)  d=image2.getPixel(x+1,y-1);
	if (a_==-1) a_=image.getPixel(x_prev-1,y_prev);
	if (b_==-1) b_=image.getPixel(x_prev,y_prev-1);
	if (c_==-1) c_=image.getPixel(x_prev-1,y_prev-1);
	if (d_==-1) d_=image.getPixel(x_prev+1,y_prev-1);
	if (e_==-1) e_=image.getPixel(x_prev,y_prev);
	if (f_==-1) f_=image.getPixel(x_prev+1,y_prev);
	if (g_==-1) g_=image.getPixel(x_prev,y_prev+1);

	
	return {a,  b,  c,  d,
			a_, b_, c_, d_,
			e_, f_, g_};
}

Image Decoder::setInitialImage(){
	Image aux=Image();
	aux.image=(int*)malloc((ancho)*(alto)*sizeof(int));

	for (int k=0;k<ancho*alto;k++) aux.image[k]=0;
	/*	definir algún criterio para esta imagen */

	aux.white=this->blanco;
	aux.width=this->ancho;
	aux.heigth=this->alto;

	return aux;
}

float Decoder::get_s(int contexto){
	return float(float(contexts[contexto].B)/float(contexts[contexto].N)); //es N o N_?
}

int Decoder::reduccionDeRango(int error, int signo,int predicted){
	//IF(J3<0,J3+$A$3,IF(J3>$A$3,J3-$A$3,J3))

	error=error*signo;

	if ((error+predicted)<0){
		error=error+range;
	} else if ((error+predicted)>range-1){
		error=error-range;
	}

	return error;
}

int Decoder::getKPrime(Racha &r){
	int T_racha, K_racha;
	T_racha=cntx[r.contexto].A_racha+(cntx[r.contexto].N_racha>>1)*r.contexto;
	for(K_racha=0; (cntx[r.contexto].N_racha<<K_racha)<T_racha; K_racha++);
	
	return K_racha;
}

int Decoder::unrice_rachas(int error,int contexto, int k){
	int map=0;
	int retorno=(error+contexto+1)>> 1;

	if ((error+contexto+1)%2==0) map=1;

	if ((k==0)and(map==1)and(2*cntx[contexto].Nn_racha<cntx[contexto].N_racha)){
	} else if ((map==1)and(2*cntx[contexto].Nn_racha>=cntx[contexto].N_racha)){
		retorno=-retorno;
	} else if ((map==1)and(k!=0)){
		retorno=-retorno;
	} else {
		if ((k!=0)or(2*cntx[contexto].Nn_racha>=cntx[contexto].N_racha)){
		} else retorno=-retorno;
	}

	 return retorno;
}

void Decoder::updateImageInterruption(Reader &reader, Racha &racha, int x,int y,int prox_, Writer &writer,int cantidad_unos, Image &image){
	if (!racha.interruption){
		int signo=1;
		if (racha.contexto==0) signo=-1;
		
		int kPrime = getKPrime(racha);
		int error_ = getError(reader,kPrime,1,cantidad_unos);
		int error  = unrice_rachas(error_,racha.contexto,kPrime);

		if (jpls2d)	error = reduccionDeRango(error*signo, 1, getPixels2D(y*ancho+prox_,image).b);
		else error = reduccionDeRango(error*signo, 1, getPixels3D(0, 0, prox_, y, image).b);
		int errorEstadisticos = clipErrorEstadisticos(error);
		
		if (jpls2d)	image.setPixel(getPixels2D(y*ancho+prox_, image).b + error, x + racha.largo, y);
		else image.setPixel(getPixels3D(0, 0, prox_, y, image).b + error, x + racha.largo, y);

		/*
		char pixel_ = getPixels3D(0, 0, prox_, y, image).b + error + '\0';
		writer.writeChar(pixel_);	//escribe el pixel en el archivo
		*/
		
		int pixel;

		if (jpls2d) pixel = getPixels2D(y*ancho+prox_, image).b + error;
		else pixel = getPixels3D(0, 0, prox_, y, image).b + error;
		writer.write(pixel, nBits);
		
		if (nuevo_debug){


			cout<<"kPrime: "<<kPrime<<endl;
			cout<<"error: "<<error_<<endl;

		}

		updateContexto_(racha.contexto, unrice_rachas(error_, racha.contexto, kPrime), error_);
	}
}

void Decoder::updateImageRacha(Racha &racha, int x,int y, Writer &writer, Image &image){
	for (int k=0;k<racha.largo;k++){
		image.setPixel(racha.pixel, x + k, y);
		
		/*
		char pixel_ =racha.pixel+'\0';
		writer.writeChar(pixel_);	//escribe el pixel en el archivo
		*/
		
		writer.write(racha.pixel, nBits);
	}
}

int Decoder::getRachaParams2(Reader &reader, int x,int &interruption_, int &cantidad_unos){
	int largo=0;
	int bit;
	int ajuste=0;
	interruption_=1;
	bool finDeRacha=false;

	while (true){
		bit=reader.read(1);
		if (bit==1){
			if ((1 << J[RUNindex])>(-largo+ancho-(x))){
				largo=ancho-(x);
			} else {
				largo=largo + (1 << J[RUNindex]);
				if (RUNindex<31) RUNindex++;
			}
			if (largo==ancho-(x)) break;
		} else {
			break;
		}
	}
	if (bit==0) {
	interruption_=0;
	for (int j=0;j<J[RUNindex];j++){
		largo=largo+(pow(2,J[RUNindex]-j-1))*reader.read(1);
	}
	ajuste = J[RUNindex];
	if (RUNindex>0) RUNindex--;
	finDeRacha=true;
	}
	cantidad_unos=ajuste+1;

	return largo;
}

void Decoder::updateImage(int pixel, int x,int y, Image &image){
	/** Agrega el pixel decodificado al array que representa la imagen */

	image.setPixel(pixel,x,y);
}

int Decoder::unRice(int error,float s, int k){
	/** Inverso de mapeo de rice */

	if ((k<=0)and(s<=-0.5)){
		if (error%2==0)	{
			int aux=error/2;
			return -aux-1;
		} else {
			int aux=((error+1)/(-2));
			return -aux-1;
		}
	} else {
		if (error%2==0)	return error/2;
			else return ((error+1)/(-2));
	}
}

/*
int Decoder::getError(Reader &reader, int k, int racha, int ajuste){
	// Devuelve como entero el error codificado

	int qMax;

	if (racha) {
		qMax=this->qMax_;
		qMax=qMax-ajuste;
	} else {
		qMax=this->qMax;
	}

	int error=0;
	int potencia=floor(pow(2,k));
	int bit=0;
	int contador=0;

	// Obtiene la cantidad de ceros que le siguen antes del primer uno,
	//es la codificación unaria del cociente entre el error y 2^k
	//while ((contador!=qMax)&&codedImage.getBit()!=1){
	//		contador++;
	//}
	while ((contador!=qMax)&&(reader.read(1)!=1)){
		contador++;
	}

	if (contador!=qMax){
		// Convierte los siguientes k bits de fileToBits en un entero,
		//que corresponden a la parte binaria del error
		if ((debug4)and(racha)) cout<<" Parte binaria: "<<endl;
		for (int j=0;j<k;j++){
			//bit=codedImage.getBit();
			bit=reader.read(1);
			potencia=potencia/2;
			error=error+bit*potencia;
		}
	} else {
		potencia=pow(2,beta);
		for (int j=0;j<beta;j++){
			//bit=codedImage.getBit();
			bit=reader.read(1);
			potencia=potencia/2;
			error=error+bit*potencia;
		}
	}
	int pot_aux=1;
	if (k>=0) pot_aux=pow(2,k); //cuando k negativo pow(2,k) es 0, y necesitamos que sea 1

	// Sumando los dos valores decodificados (cociente y resto entre 2^k) resulta el mapeo de rice
	//del error codificado
	if (contador!=qMax) error=error+contador*pot_aux;

	return error;
}
*/

int Decoder::getError(Reader &reader, int k, int racha, int ajuste){
	int qMax = (racha ? (this->qMax_ - ajuste) : (this->qMax));
	int error, cociente=0, resto=0;
	
	if(k<0) k=0;
	
	while ((cociente < qMax) && (reader.read(1) == 0)) cociente++;

	if(cociente < qMax){
		if(k>0) resto = reader.read(k);
		error = (cociente << k) + resto;
	}
	else{
		error = reader.read(beta);
	}
	
	return error;
}

void Decoder::writeHeader(Writer &writer){
	writeMagic(writer);
	writeWidth(writer);
	writeHeigth(writer);
	writeWhite(writer);
}

void Decoder::writeMagic(Writer &writer){
	writer.writeString("P5\n", 3);
}

void Decoder::writeWidth(Writer &writer){
	string temp = str_(ancho)+" ";
	int temp_l = temp.length();

	writer.writeString(temp.c_str(),temp_l);
}
void Decoder::writeHeigth(Writer &writer){
	string temp = str_(alto)+"\n";
	int temp_l = temp.length();

	writer.writeString(temp.c_str(),temp_l);
}

void Decoder::writeWhite(Writer &writer){
	string temp = str_(blanco)+"\n";
	int temp_l = temp.length();

	writer.writeString(temp.c_str(),temp_l);
}

int Decoder::fixPrediction(int predicted,int signo, int contexto){
	predicted=predicted+(contexts[contexto].C*signo);
	if (predicted< 0) predicted=0;
	if (predicted >range-1) predicted=range-1;

	return predicted;
}

void Decoder::updateContexto(int contexto, int error){
	/** Actualiza los datos N y A del contexto */
	/** Actualiza B y C */

	contexts[contexto].B=contexts[contexto].B+error;
	contexts[contexto].A=contexts[contexto].A+abs(error);

	if (contexts[contexto].N==Nmax){
		/* si el valor de N para ese contexto es igual a Nmax divide N y A entre 2 */
		contexts[contexto].N=contexts[contexto].N/2;
		contexts[contexto].N_=contexts[contexto].N_/2;
		contexts[contexto].A=floor((double)contexts[contexto].A/(double)2);
		contexts[contexto].B=floor((double)contexts[contexto].B/(double)2);
	}

	contexts[contexto].N++;	//actualiza N
	if (error<0) contexts[contexto].N_++;
	/* Actualiza A sumándole el valor absoluto de este error */
	if (contexts[contexto].B<=-contexts[contexto].N){
		contexts[contexto].B=contexts[contexto].B+contexts[contexto].N;
		if (contexts[contexto].C > -(1 + blanco)/2) contexts[contexto].C=contexts[contexto].C-1;
		if (contexts[contexto].B <= -contexts[contexto].N) contexts[contexto].B=-contexts[contexto].N+1;
	} else if (contexts[contexto].B > 0){
		contexts[contexto].B=contexts[contexto].B-contexts[contexto].N;
		if (contexts[contexto].C < blanco/2) contexts[contexto].C=contexts[contexto].C+1;
		if (contexts[contexto].B > 0) contexts[contexto].B=0;
	}
}

int Decoder::getK(int contexto){
	double AdivN_=(double)contexts[contexto].A/(double)contexts[contexto].N;

	return round(log2(AdivN_));
}

int Decoder::selectMED(grad gradients){
	int gz = abs(gradients.ga);
	int gx = abs(gradients.gb);
	int gy = abs(gradients.gc);
	int modo = 3;

	if ((gz>=gx)&&(gz>=gy)){
		modo = 0;
	}
	if ((gx>=gz)&&(gx>=gy)){
		modo = 2;
	}

	return modo;
}

Decoder::grad Decoder::getGradients3D(int modo, pixels3D pxls){
	/** Devuelve los gradientes según el modo de funcionamiento elegido:
	 * Modos:
	 * 		1 = Devuelve los gradientes de elección de predictor
	 * 		2 = Devuelve los gradientes del predictor filas-tiempo
	 * 		3 = Devuelve los gradientes del predictor columnas-tiempo
	 * 		0 o cualquier otros = Devuelve los gradientes del predictor filas-columnas
	 *  */

	grad gradients={pxls.d-pxls.b,pxls.b-pxls.c,pxls.c-pxls.a};

	if (modo == 1){
			gradients={pxls.a-pxls.a_,pxls.b-pxls.c,pxls.c-pxls.a};
	}
	if (modo == 2){
			gradients={pxls.g_-pxls.e_,pxls.e_-pxls.b_,pxls.b_-pxls.b};
	}
	if (modo == 3){
			gradients={pxls.f_-pxls.e_,pxls.e_-pxls.a_,pxls.a_-pxls.a};
	}
	if (modo == 4){
			gradients={0,pxls.b-pxls.b_,pxls.a-pxls.a_};
	}

	return gradients;
}

int Decoder::getPredictedValue(int modo, pixels3D pxls){
	/** Calcula el valor predicho según expresión de las diapositivas del curso */
	int pred;

	if (modo == 0){
		if ((pxls.c>=pxls.a)&&(pxls.c>=pxls.b)){
			if (pxls.a>pxls.b)
					pred = pxls.b;
			else pred = pxls.a;
		}else if ((pxls.c<=pxls.a)&&(pxls.c<=pxls.b)){
			if (pxls.a>pxls.b)
					pred = pxls.a;
			else pred = pxls.b;
		}else pred = (pxls.a+pxls.b-pxls.c);
	}

	if (modo == 2) {
		if ((pxls.b_>=pxls.e_)&&(pxls.b_>=pxls.b)){
			if (pxls.e_>pxls.b)
				pred = pxls.b;
			else pred = pxls.e_;
		}else if ((pxls.b_<=pxls.e_)&&(pxls.b_<=pxls.b)){
			if (pxls.e_>pxls.b)
				pred = pxls.e_;
			else pred = pxls.b;
		}else pred = (pxls.e_+pxls.b-pxls.b_);
	}

	if (modo == 3){
		if ((pxls.a_>=pxls.e_)&&(pxls.a_>=pxls.a)){
				if (pxls.e_>pxls.a)
					pred = pxls.a;
				else pred = pxls.e_;
			}else if ((pxls.a_<=pxls.e_)&&(pxls.a_<=pxls.a)){
				if (pxls.e_>pxls.a)
					pred = pxls.e_;
				else pred = pxls.a;
			}else pred = (pxls.e_+pxls.a-pxls.a_);
	}

	return pred;
}

int Decoder::getContext(grad gradients1,grad gradients2, int &signo,bool &racha){
	/*Determina el contexto
	Todos los contextos posibles se organizan en un array, donde cada elemento del array representa un contexto,
	es posible definir un mapeo entre el espacio de todos los contextos posibles y los enteros,
	para que dado un contexto haya una relación biunívoca con un elemento del arra*/

	signo=1;

	int contga, contgb,contgc,contgd,contge;

	if (gradients1.ga<=-t3) contga=0;
		else if (gradients1.ga<=-t2) contga=1;
		else if (gradients1.ga<=-t1) contga=2;
		else if (gradients1.ga<0) contga=3;
		else if (gradients1.ga==0) contga=4;
		else if (gradients1.ga<t1) contga=5;
		else if (gradients1.ga<t2) contga=6;
		else if (gradients1.ga<t3) contga=7;
		else contga=8;

	if (gradients1.gb<=-t3) contgb=0;
		else if (gradients1.gb<=-t2) contgb=1;
		else if (gradients1.gb<=-t1) contgb=2;
		else if (gradients1.gb<0) contgb=3;
		else if (gradients1.gb==0) contgb=4;
		else if (gradients1.gb<t1) contgb=5;
		else if (gradients1.gb<t2) contgb=6;
		else if (gradients1.gb<t3) contgb=7;
		else contgb=8;

	if (gradients1.gc<=-t3) contgc=0;
		else if (gradients1.gc<=-t2) contgc=1;
		else if (gradients1.gc<=-t1) contgc=2;
		else if (gradients1.gc<0) contgc=3;
		else if (gradients1.gc==0) contgc=4;
		else if (gradients1.gc<t1) contgc=5;
		else if (gradients1.gc<t2) contgc=6;
		else if (gradients1.gc<t3) contgc=7;
		else contgc=8;

	if (gradients2.gb<=-t3) contgd=0;
		else if (gradients2.gb<=-t2) contgd=1;
		else if (gradients2.gb<=-t1) contgd=2;
		else if (gradients2.gb<0) contgd=3;
		else if (gradients2.gb==0) contgd=4;
		else if (gradients2.gb<t1) contgd=5;
		else if (gradients2.gb<t2) contgd=6;
		else if (gradients2.gb<t3) contgd=7;
		else contgd=8;

	if (gradients2.gc<=-t3) contge=0;
		else if (gradients2.gc<=-t2) contge=1;
		else if (gradients2.gc<=-t1) contge=2;
		else if (gradients2.gc<0) contge=3;
		else if (gradients2.gc==0) contge=4;
		else if (gradients2.gc<t1) contge=5;
		else if (gradients2.gc<t2) contge=6;
		else if (gradients2.gc<t3) contge=7;
		else contge=8;

	if(contga<4){
		contga=8-contga;
		contgb=8-contgb;
		contgc=8-contgc;
		contgd=8-contgd;
		contge=8-contge;

		signo=-1;
	} else if((contga==4)and(contgb<4)) {
		contga=8-contga;
		contgb=8-contgb;
		contgc=8-contgc;
		contgd=8-contgd;
		contge=8-contge;

		signo=-1;
	} else if((contga==4)and(contgb==4)and(contgc<4)) {
		contga=8-contga;
		contgb=8-contgb;
		contgc=8-contgc;
		contgd=8-contgd;
		contge=8-contge;

		signo=-1;
	} else if((contga==4)and(contgb==4)and(contgc==4)and(contgd<4)) {
		contga=8-contga;
		contgb=8-contgb;
		contgc=8-contgc;
		contgd=8-contgd;
		contge=8-contge;

		signo=-1;
	} else if((contga==4)and(contgb==4)and(contgc==4)and(contgd==4)and(contge<4)) {
		contga=8-contga;
		contgb=8-contgb;
		contgc=8-contgc;
		contgd=8-contgd;
		contge=8-contge;

		signo=-1;
	}
	//mapeo elegido para representar los contextos

	if ((9*9*9*9*contga)+(9*9*9*contgb)+(9*9*contgc)==29484) racha=true;
		else racha=false;

	return (9*9*9*9*contga)+(9*9*9*contgb)+(9*9*contgc)+(9*contgd)+contge;
}


void Decoder::setContextsArray(){
	/** Forma el array con todos los contextos posibles */

	int indice=0;
	for (int k=-4;k<5;k++){
		for (int j=-4;j<5;j++){
			for (int i=-4;i<5;i++){
				for (int l=-4;l<5;l++){
					for (int m=-4;m<5;m++){

						//cout<<"blanco: "<<blanco<<endl;

						Context contexto(k,j,i,l,m,blanco);
						contexts[indice]=contexto;
						indice++;
					}
				}
			}
		}
	}
}

void Decoder::setCompParams(bool activarCompMov){


	this->activarCompMov=activarCompMov;
}


int Decoder::getP(pixels pxls){
	return floor((double)(2*pxls.a+2*pxls.b+2*pxls.c+3)/(double)6);
}

void Decoder::setMode(bool mode){

	this->jpls2d=mode;
}

int Decoder::getContext_(int x,int y, int lar, Image &image){
	if (jpls2d)	return (getPixels2D(y*ancho+x,image).a==getPixels2D(y*ancho+x+lar,image).b);
	else return (getPixels3D(0,0,x,y,image).a==getPixels3D(0,0,x+lar,y,image).b);
}

void Decoder::updateContexto_(int c, int err,int err_){
	cntx[c].updateA((err_+1-c)>>1);
	cntx[c].updateNn(err);

	if(cntx[c].N_racha==reset) {
		cntx[c].reset();
	}
	cntx[c].updateN();
}

int Decoder::clipErrorEstadisticos(int error){
	int errorEstadisticos=error;
	
	if(error > (1 + blanco)/2) errorEstadisticos -= (1 + blanco);
	else if(error < -(1 + blanco/2)) errorEstadisticos += (1 + blanco);
	
	return errorEstadisticos;
}

} /* namespace std */
