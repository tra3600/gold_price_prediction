# gold_price_prediction
programme en C++ qui prédit l'évolution du cours de l'or en 2025   retourne l'évolution du cours de l'once d'or en euros et en dollar en incluant l'évolution du taux de change euros dollars prédit lui aussi

Prédire l'évolution du cours de l'or et des taux de change est une tâche complexe qui nécessite l'utilisation de modèles économiques et de machine learning. Cependant, pour les besoins de cet exercice, nous allons simplifier le problème en utilisant des données historiques et des prédictions linéaires.

Nous allons créer un programme en C++ qui utilise des données fictives pour prédire les cours de l'or et les taux de change mois par mois pour 2025. Nous utiliserons des coefficients de tendance calculés à partir de données historiques simplifiées.

Explications
GoldPricePredictor Class:

Contient les données historiques simplifiées pour les prix de l'or en euros et les taux de change USD/EUR.
Utilise une méthode de prédiction linéaire simple basée sur la tendance historique.
Prédit les prix de l'or en euros et en dollars pour chaque mois de 2025.
Affiche les prédictions.
Prédiction Linéaire:

Calcule la tendance en utilisant les données historiques.
Projette les prix futurs en ajoutant la tendance mois par mois.
Affichage des Prédictions:

Affiche les prédictions pour chaque mois de 2025 en euros, en dollars, et les taux de change correspondants.
Compilation et Exécution
Assurez-vous que tous les fichiers sont dans le même répertoire, puis compilez et exécutez le programme avec :

g++ -o gold_price_prediction main.cpp GoldPricePredictor.cpp
./gold_price_prediction

Note
Ce programme utilise des données historiques fictives et une méthode de prédiction linéaire très simplifiée. En réalité, la prédiction des cours de l'or et des taux de change nécessite des modèles économiques sophistiqués et des techniques de machine learning. Pour des prédictions précises, il est recommandé d'utiliser des bibliothèques de machine learning et des données réelles.
