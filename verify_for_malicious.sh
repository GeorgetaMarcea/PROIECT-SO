#!/bin/bash

nume_fisier="$1"
director_izolare="$2"

# Verifică dacă fișierul există
if [ ! -f "$nume_fisier" ]; then
    echo "File not found: $nume_fisier"
    exit 1
fi

chmod 777 "$nume_fisier"

verifica_fisier_periculos() {

    numar_linii=$(wc -l < "$nume_fisier")
    numar_cuvinte=$(wc -w < "$nume_fisier")
    numar_caractere=$(wc -m < "$nume_fisier")

    count=0
    #if ["$numar_linii" -lt 3] && ["$numar_cuvinte" -gt 1000] && ["$numar_caractere" -gt 2000]; then

        cuvinte_periculoase=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")

        for cuvant in "${cuvinte_periculoase[@]}" 
        do
            if grep -q -i "$cuvant" "$nume_fisier"; then
                count=1
                break
            fi
        done
    

        #LC_ALL este o variabilă de mediu utilizată pentru controlul localizării în Linux.
        #[^[:print:]] cautam caractere care nu sunt imprimabile, adica non-ascii
    
        if LC_ALL=C grep -q '[^[:print:]]' "$nume_fisier"; then
            count=1
        fi
    #fi

    echo "$count"
}

# Verificăm dacă fișierul este periculos
este_periculos=$(verifica_fisier_periculos "$nume_fisier")

if [ "$este_periculos" -eq 1 ]; then
    echo "$nume_fisier"

    # Verificăm dacă directorul există și creăm directorul dacă nu există
    #if [ ! -d "$director_izolare" ]; then
    #    mkdir "$director_izolare"
    #    echo "Directorul $director_izolare a fost creat."
    #fi

    # Mutăm fișierul periculos în directorul de izolare
    #mv "$nume_fisier" "$director_izolare"

    #echo "Fișierul $nume_fisier a fost izolat în directorul $director_izolare."
else
    echo "Fisier: $nume_fisier -> SAFE"
fi