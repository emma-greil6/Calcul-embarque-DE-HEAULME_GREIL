double n = 0.; //Nombre de cycles
double theta0 = 0.; 
double theta1 = 0.;
double thetaN0 = 0.0;  //theta0 N+1
double thetaN1 = 0.0; //theta1 N+1
double m  = 24.;      //Nombre de valeurs

//A modifier pour l'entrainement
double eps = 51.4339;    //Objectif d'erreur
double alpha = 0.0002;    //Taux d'apprentissage

//Valeurs d'entrainements
double xi[] = {5.,5.,5.,5.,10.,10.,10.,10.,30.,30.,30.,30.,45.,45., 45.,45.,70.,70.,70.,70.,90.,90.,90.,90.};
double yi[] = {8.000,8.300,8.000,8.100,9.900,9.900,10.000,10.700,16.500,16.800,16.020, 16.150, 21.290, 21.620, 21.000, 20.700, 28.900, 28.200, 27.950, 27.810, 34.160,34.520,34.250,35.160};
//Yi en k RPM

//Todo Fonction de prédiction duty cycle => vitesse
double predictVitesse(double x) {
  return hypRLin(x);
}

//Todo Fonction de prédiction vitesse => duty cycle
double predictDuty(double y){
  // Inverse de la régression linéaire : x = (y - theta0) / theta1
  return (y - theta0) / theta1;
}

//Todo Fonction d'hypothese linéaire
double hypRLin(double x) {
  return theta0 + theta1 * x; //correspond à h(theta)
}

void setup() {
  Serial.begin(115200);
  //Todo Entrainement + prediction
  learnerMLR();

  // Exemple de prédiction
  Serial.println("--- Predictions ---");
  double testDuty = 20.0;
  double testDuty2 = 60.0;
  Serial.print("Vitesse pour duty=");
  Serial.print(testDuty);
  Serial.print("% : ");
  Serial.println(predictVitesse(testDuty), 4);
  Serial.print("Vitesse pour duty=");
  Serial.print(testDuty2);
  Serial.print("% : ");
  Serial.println(predictVitesse(testDuty2), 4);

  double testVitesse = 12.0;
  double testVitesse2 = 25.0;
  Serial.print("Duty pour vitesse=");
  Serial.print(testVitesse);
  Serial.print(" kRPM : ");
  Serial.println(predictDuty(testVitesse), 4);
  Serial.print("Duty pour vitesse=");
  Serial.print(testVitesse2);
  Serial.print(" kRPM : ");
  Serial.println(predictDuty(testVitesse2), 4);
}

void learnerMLR() {
  double erreur = 0.0; 
  double resHyp = 0.0;
  double tmpSomme0 = 0.0;
  double tmpSomme1 = 0.0;
  do {
    //Todo: Descente de gradient 
    //Regression lineaire
    erreur = 0.0;
    tmpSomme0 = 0.0;
    tmpSomme1 = 0.0;

    // Calcul des sommes pour la descente de gradient
    for (int i = 0; i < (int)m; i++) {
      resHyp = hypRLin(xi[i]);
      double diff = resHyp - yi[i];
      tmpSomme0 += diff;
      tmpSomme1 += diff * xi[i];
      erreur += diff * diff;
    }

    // Mise à jour simultanée des thetas
    thetaN0 = theta0 - alpha * (1.0 / m) * tmpSomme0;
    thetaN1 = theta1 - alpha * (1.0 / m) * tmpSomme1;
    theta0 = thetaN0;
    theta1 = thetaN1;

    // Erreur quadratique moyenne (MSE)
    erreur = erreur / 2*m;

    n++;

    Serial.print("Erreur: ");
    Serial.println(erreur,6);
  } while (eps < erreur);
  Serial.print("Theta0: ");
  Serial.println(theta0,6);
  Serial.print("Theta1: ");
  Serial.println(theta1,6);
  Serial.print("Nombre de cycles: ");
  Serial.println(n);
}

//Rien dans la loop
void loop() {}