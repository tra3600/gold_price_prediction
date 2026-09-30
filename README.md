# gold_price_prediction

Programme C++17 (sans dépendance) qui prédit l'évolution mensuelle du cours de l'once d'or en **euros** et en **dollars**, en incluant l'évolution prévue du taux de change USD/EUR.

## Fonctionnalités
- Historique **en ligne** (`--online`) : or (XAUUSD) et EURUSD mensuels depuis [Stooq](https://stooq.com) (sans clé d'API), or converti en EUR.
- Ou historique depuis un **fichier CSV** (ou données d'exemple fictives intégrées).
- Deux modèles : **régression linéaire (moindres carrés)** et **lissage exponentiel de Holt** (paramètres optimisés automatiquement).
- Mode `auto` : choisit le meilleur modèle par **backtest** sur les derniers mois (MAE / RMSE / MAPE affichés).
- **Intervalles de confiance à 95 %** pour chaque mois prévu.
- Horizon configurable, **export CSV** des prévisions, validation des données et messages d'erreur clairs.
- Tests unitaires (`ctest`).

## Compilation
```
cmake -S . -B build && cmake --build build
ctest --test-dir build          # tests
```
ou sans CMake : `g++ -std=c++17 -O2 -o gold_price_prediction main.cpp GoldPricePredictor.cpp`

## Utilisation
```
./build/gold_price_prediction                          # données d'exemple, 12 mois
./build/gold_price_prediction --data data/sample.csv --horizon 6 --model holt --output prev.csv
```
Options : `--data`, `--online`, `--months`, `--source-url`, `--save-data`, `--horizon` (1–120), `--model linear|holt|auto`, `--output`, `--help`.

### Données en ligne
```
./build/gold_price_prediction --online --months 60 --horizon 12 --save-data historique.csv
./build/gold_price_prediction --data historique.csv        # réutilisation hors ligne
```
Nécessite `curl` et l'accès à `stooq.com`. Le mois en cours (incomplet) est exclu. Prix EUR = XAUUSD / EURUSD. `--source-url` permet de pointer vers un miroir ou un serveur compatible (format CSV Stooq : `Date,Open,High,Low,Close,Volume`, paramètres `s=<symbole>&i=m`).

### Format du CSV
```
date,gold_eur,usd_per_eur
2024-01,1920,1.09
```
Date `YYYY-MM`, prix de l'once en EUR, nombre de USD pour 1 EUR. Minimum 6 mois. Les lignes `#` sont ignorées.

## Corrections par rapport à la version initiale
- La tendance utilisait seulement les deux extrémités des données ; elle utilise maintenant tous les points.
- Les valeurs prédites étaient réinjectées dans la boucle de prévision (dérive de la projection).
- Plus de valeurs codées en dur uniquement, plus d'année figée « 2025 », plus de binaire `.exe` versionné.

## Avertissement
Les données intégrées sont **fictives** et les modèles sont des extrapolations statistiques simples : ce n'est pas un conseil financier. Utilisez des données réelles pour des résultats significatifs.
