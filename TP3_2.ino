float n = 0.;  //Nombre de cycles
float theta0 = 0.; 
float theta1 = 0.;
float thetaN0 = 0.0; //theta0 N+1
float thetaN1 = 0.0; //theta1 N+1
float m  = 10.;      //Nombre de valeurs

//A modifier l'entrainement

float eps = 0.0005;    //Objectif d'erreur
float alpha = 0.00001; //Taux d'apprentissage


//Valeurs d'entrainements
float xi[] = {-255, 70, -128,-60, -50, 80, 30, 128, 200, -80};
float yi[] = {0,1,0,0,0,1,1,1,1,0};


//Todo fonction de prediction
float predict(double x) {
  return hypRLogBin(x);
}

//Todo fonction hypothese logistique binaire
float hypRLogBin(double x) {
  return 1.0/(1.0 + exp(-theta0-theta1*x));
}

void setup() {
  Serial.begin(115200);
  //Todo Entrainement + prediction
  learnerMLRLog();
  Serial.println("--- Predictions ---");
  double test = 0;
  Serial.println("Valeur de Test");
  Serial.println(test);
  float p = predict(test);
  Serial.println("Valeur de p");
  Serial.println(p);
  Serial.println("Direction :");
  if (p < 0.49){
    Serial.println("Vers la gauche");
  } 
  else if (p <= 0.51 && p>=0.49){
    Serial.println("Tout droit");
  }
  else if (p>0.51){
    Serial.println("Vers la droite");
  }

}

void learnerMLRLog() {
  float erreur = 0.0; 
  float resLog = 0.0;
  float tmpSomme0 = 0.0;
  float tmpSomme1 = 0.0;
  do {
    //Todo: Descente de gradient 
    //Regression logistique
    erreur = 0.0;
    tmpSomme0 = 0.0;
    tmpSomme1 = 0.0;

    for(int i = 0; i<(int)m; i++){
      resLog = hypRLogBin(xi[i]);
      double diff = resLog - yi[i];
      tmpSomme0 += (resLog-yi[i])*resLog*(1-resLog);
      tmpSomme1 += ((resLog-yi[i])*resLog*(1-resLog)) * xi[i];
      erreur += diff*diff;
    }

    thetaN0 = theta0 - alpha*(1.0/m)*tmpSomme0;
    thetaN1 = theta1 - alpha * (1.0 / m) * tmpSomme1;
    theta0 = thetaN0;
    theta1 = thetaN1;

    erreur = erreur/(2*m);

    n++;

    Serial.print("Erreur: ");
    Serial.println(erreur,6);
  } while (eps<erreur);
  Serial.print("Theta0: ");
  Serial.println(theta0,6);
  Serial.print("Theta1: ");
  Serial.println(theta1,6);
  Serial.print("Nombre de cycles: ");
  Serial.println(n);
}

//Rien dans la loop
void loop() {}