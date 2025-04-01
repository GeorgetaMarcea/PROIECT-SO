# Proiect SO

# Descriere
Acest proiect are ca scop monitorizarea modificărilor din directoare, crearea de snapshot-uri pentru urmărirea acestora și analiza fișierelor suspecte din punct de vedere al securității. Implementarea presupune utilizarea proceselor pentru execuția în paralel și diverse tehnici pentru detectarea fișierelor corupte sau potențial periculoase.

## Săptămâna 1 – Definirea proiectului și implementarea snapshot-urilor
Proiectul vizează monitorizarea schimbărilor dintr-un director specificat de utilizator. La fiecare rulare a programului, se creează un snapshot care conține metadatele tuturor fișierelor și subdirectoarelor.

### Cerințe:
- Programul primește ca argument în linia de comandă un director și creează un snapshot pentru conținutul acestuia.
- Snapshot-ul stochează informații despre fiecare fișier și director (ex: permisiuni, mărime, dată modificare).
- La fiecare rulare, snapshot-ul este actualizat cu modificările identificate.

## Săptămâna 2 – Extinderea suportului pentru multiple directoare

### Cerințe:
- Programul acceptă până la 10 directoare ca argumente în linia de comandă.
- Se ignoră argumentele care nu sunt directoare.
- Se implementează o opțiune `-o` pentru specificarea directorului de ieșire unde se vor stoca snapshot-urile.
- Se adaugă funcționalitatea de comparare a snapshot-urilor pentru a detecta modificările.

Exemplu de rulare:
./program_exe -o output_dir dir1 dir2 dir3


## Săptămâna 3 – Procesare paralelă folosind fork()
Pentru a îmbunătăți performanța, fiecare director analizat este procesat într-un proces copil.

### Cerințe:
- Se creează un proces separat pentru fiecare director monitorizat.
- Programul afișează PID-ul fiecărui proces copil și codul său de ieșire.
- Snapshot-urile sunt generate în paralel pentru eficiență maximă.

Exemplu de output:
Captura pentru Directorul 1 creată cu succes.
Captura pentru Directorul 2 creată cu succes.
Procesul Copil 1 s-a încheiat cu PID 123 și cod de ieșire 0.


## Săptămâna 4 – Optimizarea stocării snapshot-urilor

### Cerințe:
- Snapshot-urile sunt salvate într-un format compact.
- Se implementează o metodă de reducere a redundanței pentru a economisi spațiu.


## Săptămâna 5 – Implementarea jurnalizării (logging)

### Cerințe:
- Se adaugă un sistem de logare a evenimentelor.
- Fiecare acțiune (creare snapshot, modificare detectată) este înregistrată într-un fișier de log.
- 

## Săptămâna 6 – Implementarea rollback-ului

### Cerințe:
- Se introduce posibilitatea de a reveni la un snapshot anterior în cazul unor modificări nedorite.
- Se implementează un mecanism de restaurare a metadatelor unui fișier sau director.


## Săptămâna 7 – Implementarea notificărilor

### Cerințe:
- Se adaugă un mecanism de notificare în timp real pentru modificările detectate.
- Utilizatorul poate primi alerte dacă un fișier este șters, modificat sau adăugat.


## Săptămâna 8 – Optimizări de performanță

### Cerințe:
- Reducerea timpului de procesare prin folosirea unor structuri de date eficiente.
- Implementarea unui algoritm de comparare mai rapid pentru snapshot-uri.


## Săptămâna 9 – Analiza fișierelor suspecte
Această etapă introduce un mecanism de securitate pentru identificarea și izolarea fișierelor potențial periculoase.

### Cerințe:
- Se verifică permisiunile fișierelor; dacă toate drepturile sunt lipsă, fișierul este considerat suspect.
- Se creează un proces separat pentru analiza conținutului fișierelor suspecte.
- Se verifică dacă fișierul conține termeni asociați cu malware sau caractere non-ASCII.
- Fișierele suspecte sunt mutate într-un director special de izolare (`izolated_space_dir`).

Exemplu de rulare:
./program_exe -o output_dir -s izolated_space_dir dir1 dir2 dir3


## Săptămâna 10 – Extinderea funcționalităților de securitate

### Cerințe:
- Se adaugă o metodă de raportare a fișierelor suspecte.
- Se implementează un mecanism de carantină, în care fișierele izolate pot fi analizate ulterior fără a afecta sistemul.
- Se optimizează procesul de scanare a fișierelor pentru a detecta amenințările mai rapid.

