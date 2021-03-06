 /**
  @file Context.cpp
  @brief Se preparan los contextos

  Se inicializan con A = 8 y N = 1, como pide la letra.

  @author Felipe Tambasco, Mauro Barbosa
  @date Feb, 2017

*/

#include "Context.h"

namespace std {

	//esta clase modela un contexto

Context::Context() {


}

Context::Context(int q1,int q2,int q3) {

	//constructor

	this->q1=q1;
	this->q2=q2;
	this->q3=q3;

	/* Se inicializan A y N según los valores que se indican en la letra del proyecto */
	this->A=8;
	this->N=1;

}

Context::~Context() {

}

} /* namespace std */
