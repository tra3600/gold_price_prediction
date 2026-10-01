# gold_price_prediction

Programme C++17 qui prédit mois par mois l'évolution du cours de l'once d'or en euros et en dollars, ainsi que du taux de change USD/EUR, avec un intervalle de confiance à 95 %.

## Méthode

- Régression linéaire (moindres carrés) sur **toutes** les données historiques, séparément pour l'or (EUR) et le taux USD/EUR.
- Prix en USD = prix en EUR × taux prédit.
- Intervalles de prédiction à ~95 % (s'élargissent avec l'horizon) ; l'intervalle USD combine les bornes or et taux (volontairement conservateur).
- Affiche la pente (tendance mensuelle) et le R² de chaque modèle.

> Les données intégrées sont **fictives**. Une droite de tendance ne remplace pas un vrai modèle économique : à n'utiliser que comme illustration.

## Compilation

```bash
cmake -S . -B build && cmake --build build
ctest --test-dir build          # tests unitaires
```

ou simplement : `g++ -std=c++17 -o gold_price_prediction main.cpp GoldPricePredictor.cpp`

## Utilisation

```bash
./build/gold_price_prediction                          # 12 mois à partir de 2025-01, données d'exemple
./build/gold_price_prediction --mois 24 --debut 2025-06
./build/gold_price_prediction --csv historique.csv --sortie predictions.csv
```

Format du CSV d'entrée (en-tête optionnel, ≥ 3 lignes) :

```
mois,or_eur,taux_usd_eur
2024-01,1850.2,1.09
2024-02,1862.7,1.08
```
