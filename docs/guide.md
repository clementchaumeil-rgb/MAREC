# MAREC — Guide utilisateur

**Version 1.0** · Copyright © 2026 Saya

---

## Table des matières

1. [Présentation](#1-présentation)
2. [Installation](#2-installation)
3. [Prérequis](#3-prérequis)
4. [Démarrage rapide](#4-démarrage-rapide)
5. [Guide détaillé de l'interface](#5-guide-détaillé-de-linterface)
   - [5.1 Connexion](#51-connexion)
   - [5.2 Sélection des pistes](#52-sélection-des-pistes)
   - [5.3 Aperçu](#53-aperçu)
   - [5.4 Renommage en cours](#54-renommage-en-cours)
   - [5.5 Résultats](#55-résultats)
6. [Convention de nommage](#6-convention-de-nommage)
7. [Importer des marqueurs](#7-importer-des-marqueurs)
8. [Exporter les clips](#8-exporter-les-clips)
9. [Ligne de commande (CLI)](#9-ligne-de-commande-cli)
10. [Dépannage et FAQ](#10-dépannage-et-faq)
11. [Informations techniques](#11-informations-techniques)

---

## 1. Présentation

MAREC est un outil macOS conçu pour Audi'Art qui automatise le renommage des clips dans Pro Tools en se basant sur les marqueurs de la session. Une tâche qui prenait environ deux heures par semaine — renommer manuellement chaque clip pour refléter sa position par rapport aux marqueurs — s'effectue désormais en quelques secondes.

**Convention de nommage.** MAREC renomme chaque clip selon le format `NomPiste - NomMarqueur`. Lorsque plusieurs clips se trouvent sous le même marqueur sur la même piste, un suffixe numéroté est ajouté : `_01`, `_02`, etc. Les clips situés avant le premier marqueur de la session sont ignorés.

**Idempotence.** Vous pouvez relancer MAREC autant de fois que vous le souhaitez sur la même session : les clips déjà correctement nommés ne seront pas modifiés. L'opération est donc sans risque.

**Fonctionnalités principales :**

- Connexion automatique à Pro Tools via le protocole PTSL
- Sélection des pistes à traiter (avec recherche et filtrage)
- Aperçu complet des actions de renommage avant exécution
- Renommage des clips dans Pro Tools (métadonnées de session)
- Renommage optionnel des fichiers audio source sur disque
- Import de marqueurs depuis un export texte d'une autre session
- Export des clips renommés au format WAV 24-bit 48 kHz
- Mode ligne de commande pour l'automatisation et le scripting

---

## 2. Installation

### Configuration requise

- macOS 15 (Sequoia) ou version ultérieure
- Pro Tools avec le protocole PTSL activé (Pro Tools standard ou Ultimate)

### Installation depuis le .pkg

1. Double-cliquez sur le fichier `MAREC.pkg` téléchargé.
2. Suivez les instructions de l'installeur.
3. MAREC s'installe dans le dossier `/Applications`.
4. Ouvrez MAREC depuis le Launchpad ou le dossier Applications.

> **💡 Astuce** — Glissez l'icône MAREC dans votre Dock pour y accéder rapidement.

### Désinstallation

Faites glisser MAREC depuis le dossier Applications vers la Corbeille. Pour supprimer également les fichiers de logs, supprimez le dossier `~/Library/Logs/MAREC/`.

---

## 3. Prérequis

> **⚠️ Attention** — Pro Tools doit être lancé avec une session ouverte *avant* de démarrer MAREC. MAREC ne peut pas ouvrir Pro Tools ni ouvrir une session à votre place.

### Marqueurs

Les marqueurs de la session définissent les noms qui seront attribués aux clips. Ils doivent être placés dans la timeline *avant* les clips que vous souhaitez nommer. Les clips situés avant le premier marqueur seront ignorés.

> **ℹ️ Note** — Si votre session ne contient pas de marqueurs, vous pouvez les importer depuis un fichier d'export texte d'une autre session. Consultez la [section 7 — Importer des marqueurs](#7-importer-des-marqueurs).

### API Privée (optionnel)

MAREC fonctionne en mode standard (API publique), mais offre de meilleures performances avec l'API Privée de Pro Tools. Pour l'activer :

1. Dans Pro Tools, ouvrez **Setup → Preferences**.
2. Onglet **Operation**.
3. Cochez **Enable Private API**.
4. Redémarrez Pro Tools.

MAREC détecte automatiquement la disponibilité de l'API Privée et l'utilise si elle est activée. Aucune configuration supplémentaire n'est nécessaire.

---

## 4. Démarrage rapide

MAREC fonctionne en 5 étapes séquentielles :

```
Connexion → Sélection des pistes → Aperçu → Renommage → Résultats
```

### Procédure express

1. Lancez Pro Tools et ouvrez votre session.
2. Lancez MAREC.
3. Cliquez sur **"Connecter"** — MAREC se connecte à Pro Tools.
4. Sélectionnez les pistes audio à traiter (ou utilisez **"Tout selectionner"**).
5. Cliquez sur **"Apercu"** — MAREC analyse les clips et les marqueurs.
6. Vérifiez le tableau d'aperçu : le nom actuel (en rouge) et le nouveau nom (en vert).
7. Cliquez sur **"Renommer N clip(s)"** pour confirmer.
8. Consultez le résumé des résultats.

> **💡 Astuce** — L'aperçu vous permet de vérifier chaque action avant de confirmer le renommage. Aucune modification n'est effectuée tant que vous n'avez pas cliqué sur le bouton de renommage.

---

## 5. Guide détaillé de l'interface

### 5.1 Connexion

L'écran de connexion est le point d'entrée de MAREC. Il affiche le message *"Connexion a Pro Tools"* et vous invite à vous assurer que Pro Tools est ouvert avec une session active.

**Bouton "Connecter"** — Lance la connexion vers Pro Tools via le protocole PTSL sur `localhost:31416`. Pendant la connexion, le bouton affiche *"Connexion..."* avec un indicateur d'activité.

**Bouton "Importer Marqueurs"** — Ouvre un sélecteur de fichier pour importer des marqueurs depuis un export texte Pro Tools. Ce bouton est disponible à tout moment, même avant la connexion. Consultez la [section 7](#7-importer-des-marqueurs) pour les détails.

L'encadré d'information en bas de l'écran rappelle que *"MAREC se connecte via PTSL (localhost:31416)"* et que *"Pro Tools doit etre lance avant de connecter."*

**Erreurs possibles :**

| Situation | Message |
|---|---|
| Pro Tools n'est pas lancé | Erreur de connexion PTSL |
| Aucune session ouverte | Erreur de connexion ou session introuvable |

En cas d'erreur, un bandeau rouge s'affiche sous les boutons avec le détail du problème.

---

### 5.2 Sélection des pistes

Après connexion, MAREC affiche la liste des pistes audio de la session.

**Barre d'information de session** — En haut de l'écran, trois indicateurs affichent :
- La fréquence d'échantillonnage (ex. `48000 Hz`)
- Le nombre de marqueurs détectés (ex. `12 marqueur(s)`)
- Le nombre de pistes audio (ex. `8 piste(s)`)

**Liste des pistes** — Chaque piste est affichée avec une case à cocher. Cliquez sur une piste pour la sélectionner ou la désélectionner. Le titre de section est *"Pistes audio"*.

**Recherche** — Lorsque la session contient plus de 10 pistes, un champ de recherche *"Filtrer les pistes..."* apparaît pour trouver rapidement une piste par son nom.

**Sélection groupée** — Le lien **"Tout selectionner"** / **"Tout deselectionner"** en haut à droite permet de basculer la sélection de toutes les pistes.

**Panneau "Marqueurs detectes"** — Un panneau dépliable affiche la liste des marqueurs de la session. Cliquez dessus pour le développer et voir chaque marqueur avec son numéro, son nom et son timecode. Lorsqu'il est replié, les premiers marqueurs sont affichés sous forme de pastilles.

**Compteur** — En bas à gauche, le compteur *"N piste(s)"* indique le nombre de pistes actuellement sélectionnées.

**Navigation :**
- **"Retour"** — Revenir à l'écran de connexion.
- **"Apercu"** — Lancer l'analyse des clips pour les pistes sélectionnées. Ce bouton est désactivé si aucune piste n'est sélectionnée.

---

### 5.3 Aperçu

L'écran d'aperçu présente un résumé complet des actions de renommage avant exécution.

**Barre de résumé** — Quatre cartes affichent les compteurs :
- **Total clips** — Nombre total de clips analysés
- **A renommer** — Clips qui seront renommés (en orange)
- **Deja corrects** — Clips dont le nom correspond déjà au marqueur (en vert)
- **Ignores (avant 1er marqueur)** — Clips situés avant le premier marqueur (affiché uniquement si > 0)

**Tableau des actions** — Sous le titre *"Actions de renommage"*, un tableau détaille chaque action avec les colonnes :

| Colonne | Description |
|---|---|
| **Piste** | Nom de la piste audio |
| **Nom actuel** | Nom actuel du clip (affiché en rouge) |
| → | Flèche de transformation |
| **Nouveau nom** | Nom après renommage (affiché en vert) |
| **Position** | Position du clip dans la timeline |

Lorsque plusieurs pistes sont sélectionnées, les actions sont regroupées par piste dans des sections dépliables. Chaque section indique le nombre d'actions à renommer et le nombre de clips déjà corrects.

**Recherche et filtrage** — Si le nombre d'actions dépasse 10, un champ *"Rechercher un clip..."* et un bouton filtre **"A renommer"** apparaissent pour affiner l'affichage.

**Si tous les clips sont déjà correctement nommés**, l'écran affiche *"Aucun clip a renommer"* avec le message *"Tous les clips sont deja correctement nommes."*

**Options** — Deux cases à cocher sont disponibles :
- ☐ **"Renommer aussi les fichiers audio"** — Renomme également les fichiers audio source sur le disque dur, et pas seulement les clips dans la session Pro Tools.
- ☐ **"Exporter apres renommage"** — Lance automatiquement l'export des clips après le renommage.

> **⚠️ Attention** — L'option *"Renommer aussi les fichiers audio"* modifie les fichiers sur le disque. Les autres sessions Pro Tools qui référencent ces fichiers verront également le changement de nom.

**Bouton "Renommer N clip(s)"** — Lance l'exécution du renommage. Le nombre N correspond au nombre total d'actions.

**Bouton "Retour"** — Revenir à la sélection des pistes.

---

### 5.4 Renommage en cours

L'écran d'exécution affiche une animation pendant le renommage des clips.

Le titre *"Renommage en cours..."* est affiché avec un indicateur visuel animé. Sous le titre, la progression en temps réel indique la piste en cours de traitement (ex. `2/5 Track 1`).

Le message *"Ne fermez pas Pro Tools pendant l'operation."* rappelle de ne pas interrompre le processus.

> **⚠️ Attention** — Ne fermez ni Pro Tools ni MAREC pendant le renommage. Une interruption pourrait laisser certains clips partiellement renommés.

---

### 5.5 Résultats

L'écran de résultats affiche un résumé chiffré de l'opération.

**Résumé** — Quatre indicateurs :
- **Renommes** (en vert) — Nombre de clips renommés avec succès
- **Erreurs** (en rouge) — Nombre de clips en erreur
- **Total** — Nombre total de clips traités
- **Succes** ou **Partiel** — Statut global de l'opération

**Section "Erreurs"** — Si des erreurs sont survenues, cette section est dépliée par défaut et liste chaque clip en erreur avec le détail de l'erreur. Chaque ligne affiche l'ancien nom, le nouveau nom prévu, et le message d'erreur.

**Section "Renommes avec succes"** — Liste les clips renommés avec succès. Si la liste dépasse 10 éléments, un champ *"Rechercher un clip..."* permet de filtrer les résultats.

**Section "Export"** — Si l'export a été activé, cette section affiche le statut de l'export pour chaque piste avec un indicateur **OK** ou **Erreur**.

**Boutons :**
- **"Nouvelle session"** — Réinitialise MAREC pour travailler sur une autre session (ou la même après modifications).
- **"Exporter les clips"** — Lance l'export des clips renommés (affiché uniquement si l'export n'a pas déjà été effectué).

---

## 6. Convention de nommage

MAREC applique la convention suivante pour nommer les clips :

```
NomPiste - NomMarqueur
```

**Exemple :** Un clip sur la piste *"Voix"* situé après le marqueur *"Scène 3"* sera renommé `Voix - Scène 3`.

### Suffixes de duplication

Lorsque plusieurs clips se trouvent sous le même marqueur sur la même piste, un suffixe numéroté est ajouté à partir du deuxième clip :

| Clip | Nom attribué |
|---|---|
| 1er clip | `Voix - Scène 3` |
| 2e clip | `Voix - Scène 3_01` |
| 3e clip | `Voix - Scène 3_02` |

### Clips avant le premier marqueur

Les clips situés avant le premier marqueur de la session sont ignorés par MAREC. Ils ne sont ni renommés ni comptabilisés dans les actions de renommage — ils apparaissent dans le compteur *"Ignores (avant 1er marqueur)"* de l'aperçu.

### Idempotence

Si un clip porte déjà le nom correct (exactement `NomPiste - NomMarqueur` ou avec un suffixe `_NN`), il n'est pas modifié. Cela signifie que vous pouvez relancer MAREC sur la même session sans risque de double renommage.

### Stéréo

Les canaux gauche (L) et droit (R) d'un clip stéréo sont comptés comme un seul clip. MAREC déduplique automatiquement les canaux pour éviter les doublons.

---

## 7. Importer des marqueurs

L'import de marqueurs est utile lorsque votre session de travail ne contient pas de marqueurs, ou lorsque vous souhaitez récupérer les marqueurs d'une autre session (par exemple, la session de tournage vers la session de mix).

### Étape 1 : Générer le fichier d'export dans Pro Tools

1. Ouvrez la session source (celle qui contient les marqueurs) dans Pro Tools.
2. Allez dans **File → Export → Session Info as Text…**
3. Vérifiez que l'option **Include Markers** est active.
4. Choisissez un emplacement et cliquez sur **OK**.
5. Le fichier généré est un fichier `.txt` contenant les informations de la session.

> **ℹ️ Note** — Le format attendu est un fichier texte contenant une section `M A R K E R S  L I S T I N G`. MAREC ignore les autres sections du fichier.

### Étape 2 : Importer dans MAREC

1. Lancez MAREC.
2. Sur l'écran de connexion, cliquez sur **"Importer Marqueurs"**.
3. Sélectionnez le fichier `.txt` exporté depuis Pro Tools.
4. MAREC affiche un aperçu de l'import dans la fenêtre **"Importer des Marqueurs"**.
5. Vérifiez le résumé : session source, fréquence d'échantillonnage, et compteurs.
6. Cliquez sur **"Importer"** pour créer les marqueurs dans la session active.

### Aperçu de l'import

L'aperçu affiche les informations suivantes :
- **Source** — Nom de la session d'origine et fréquence d'échantillonnage
- **Dans le fichier** — Nombre total de marqueurs trouvés dans le fichier
- **A creer** — Nombre de marqueurs qui seront créés (en vert)
- **Existants** — Nombre de marqueurs déjà présents dans la session (en orange)

### Option "Supprimer les marqueurs existants"

> **⚠️ Attention** — Cette option supprime tous les marqueurs existants de la session avant l'import. Cette action est irréversible dans MAREC (mais annulable via Cmd+Z dans Pro Tools immédiatement après).

Activez cette option si vous souhaitez remplacer intégralement les marqueurs de la session par ceux du fichier importé.

### Résultat de l'import

Après l'import, MAREC affiche le message *"Import termine"* avec les compteurs :
- **Crees** — Marqueurs créés avec succès
- **Ignores** — Marqueurs ignorés (doublons)
- **Erreurs** — Marqueurs en erreur (le cas échéant)
- **Supprimes** — Marqueurs supprimés (si l'option était activée)

> **💡 Astuce** — Les marqueurs dont le numéro existe déjà dans la session sont automatiquement ignorés pour éviter les doublons.

---

## 8. Exporter les clips

MAREC peut exporter les clips renommés sous forme de fichiers audio individuels.

### Deux façons d'exporter

1. **À l'aperçu** — Cochez l'option **"Exporter apres renommage"** avant de lancer le renommage. L'export s'effectue automatiquement après le renommage.
2. **Aux résultats** — Cliquez sur le bouton **"Exporter les clips"** sur l'écran de résultats.

### Format d'export

Les fichiers sont exportés au format fixe **WAV 24-bit 48 kHz**. Ce format garantit une qualité professionnelle compatible avec tous les workflows de post-production.

### Destination

Par défaut, les fichiers sont exportés dans le sous-dossier `Bounced Files/` du dossier de la session Pro Tools.

> **ℹ️ Note** — Pro Tools ne peut pas exporter vers certains répertoires système comme `/tmp/`. Utilisez le dossier de session ou un dossier dans votre répertoire utilisateur.

---

## 9. Ligne de commande (CLI)

MAREC dispose d'un mode ligne de commande qui offre les mêmes fonctionnalités que l'interface graphique. Le CLI est utile pour l'automatisation et le scripting.

### Mode interactif

```bash
marec [options]
```

| Option | Description |
|---|---|
| `--dry-run` | Aperçu du renommage sans exécution |
| `--all-tracks` | Traiter toutes les pistes audio (sans sélection interactive) |
| `--rename-file` | Renommer aussi les fichiers audio source sur disque |
| `--export` | Exporter les clips après renommage |
| `--output-dir CHEMIN` | Dossier de destination pour l'export (défaut : dossier de la session) |
| `--format FORMAT` | Format d'export : `wav` ou `aiff` (défaut : `wav`) |
| `--bit-depth PROFONDEUR` | Profondeur de bits : `16`, `24` ou `32` (défaut : `24`) |
| `--help`, `-h` | Afficher l'aide |

**Exemple :** Aperçu du renommage de toutes les pistes sans exécution :

```bash
marec --dry-run --all-tracks
```

### Import de marqueurs

```bash
marec --import-markers fichier.txt [--clear-markers] [--dry-run]
```

| Option | Description |
|---|---|
| `--import-markers FICHIER` | Importer les marqueurs depuis un fichier d'export texte Pro Tools |
| `--clear-markers` | Supprimer tous les marqueurs existants avant l'import |
| `--dry-run` | Aperçu de l'import sans créer les marqueurs |

**Exemple :** Importer des marqueurs en supprimant les existants :

```bash
marec --import-markers session_tournage.txt --clear-markers
```

### Mode JSON

Le mode JSON (`--json --step <étape>`) est utilisé par l'interface graphique pour communiquer avec le moteur C++. Il retourne des réponses JSON structurées pour chaque étape. Ce mode est principalement destiné à l'intégration et au développement.

```bash
marec --json --step preview --tracks "Voix,Ambiance"
```

---

## 10. Dépannage et FAQ

### Problèmes courants

#### 1. Connexion échouée

**Cause :** Pro Tools n'est pas lancé, aucune session n'est ouverte, ou le protocole PTSL n'est pas disponible.

**Solution :**
- Vérifiez que Pro Tools est lancé et qu'une session est ouverte.
- Vérifiez que vous utilisez Pro Tools standard ou Ultimate (Pro Tools First ne supporte pas le PTSL).
- Redémarrez Pro Tools si le problème persiste.

#### 2. Aucun marqueur trouvé

**Cause :** La session ne contient pas de marqueurs (Memory Locations de type Marker).

**Solution :**
- Créez des marqueurs dans Pro Tools (Entrée → Marqueurs ou raccourci clavier).
- Importez des marqueurs depuis un fichier d'export texte (voir [section 7](#7-importer-des-marqueurs)).
- Vérifiez que vos Memory Locations sont bien de type *Marker* et non *Selection* ou *None*.

#### 3. Aucun clip à renommer

**Cause :** Tous les clips sont déjà correctement nommés (idempotence).

**Solution :** C'est un comportement normal si MAREC a déjà été exécuté sur cette session. Si vous avez ajouté de nouveaux clips, vérifiez qu'ils se trouvent après le premier marqueur.

#### 4. Clips ignorés (avant le premier marqueur)

**Cause :** Certains clips commencent avant la position du premier marqueur dans la timeline.

**Solution :**
- Ajoutez un marqueur avant ces clips si vous souhaitez les inclure dans le renommage.
- Déplacez les clips après le premier marqueur existant.

#### 5. Erreurs partielles lors du renommage

**Cause :** Certains clips n'ont pas pu être renommés (conflit de nom, fichier verrouillé, etc.).

**Solution :**
- Consultez la section *"Erreurs"* dans l'écran de résultats pour le détail de chaque erreur.
- Vérifiez les fichiers de logs pour plus d'informations (voir ci-dessous).
- Corrigez le problème dans Pro Tools et relancez MAREC — les clips déjà renommés ne seront pas retouchés.

#### 6. Binaire introuvable (GUI)

**Cause :** L'interface graphique ne trouve pas l'exécutable CLI `marec`.

**Solution :**
- Réinstallez MAREC depuis le .pkg.
- Vérifiez que l'application n'a pas été déplacée en dehors du dossier Applications.

#### 7. L'export échoue

**Cause :** Pro Tools ne peut pas écrire dans le dossier de destination.

**Solution :**
- Vérifiez que le dossier `Bounced Files/` existe dans le dossier de la session.
- N'utilisez pas de dossiers système comme `/tmp/` comme destination.
- Vérifiez les permissions d'écriture sur le dossier de destination.

### Fichier de logs

MAREC enregistre ses opérations dans un fichier de log :

```
~/Library/Logs/MAREC/marec.log
```

Pour consulter les logs en temps réel, ouvrez **Console.app** et filtrez par le sous-système `com.audiart.marec`. Vous pouvez aussi utiliser le terminal :

```bash
tail -f ~/Library/Logs/MAREC/marec.log
```

Le fichier de log contient les invocations CLI, les réponses de Pro Tools, les transitions d'étapes, et les éventuelles erreurs détaillées.

### FAQ

#### MAREC modifie-t-il le contenu audio des fichiers ?

Non. Par défaut, MAREC modifie uniquement les métadonnées de la session Pro Tools (le nom des clips). Le contenu audio n'est jamais altéré. Si vous activez l'option *"Renommer aussi les fichiers audio"*, seul le nom des fichiers sur disque est modifié — le contenu audio reste identique.

#### Puis-je annuler un renommage ?

Oui. Utilisez **Cmd+Z** (Annuler) dans Pro Tools immédiatement après le renommage. Pro Tools enregistre le renommage dans son historique d'annulation comme n'importe quelle autre opération.

#### Puis-je relancer MAREC sur la même session ?

Oui, c'est sans risque. MAREC est idempotent : les clips déjà correctement nommés sont automatiquement ignorés. Seuls les clips dont le nom ne correspond pas au marqueur sont renommés.

#### Pro Tools First est-il compatible ?

Non. Pro Tools First ne supporte pas le protocole PTSL (Pro Tools Scripting Language), qui est nécessaire pour la communication avec MAREC. Vous avez besoin de Pro Tools standard ou Ultimate.

#### Quelle est la différence entre renommer un clip et renommer un fichier audio ?

- **Clip** : Nom de la référence dans la session Pro Tools. Le renommage modifie uniquement la session (fichier `.ptx`). D'autres sessions utilisant le même fichier audio ne sont pas affectées.
- **Fichier audio** : Nom du fichier `.wav` ou `.aiff` sur le disque dur. Si vous activez l'option *"Renommer aussi les fichiers audio"*, le fichier physique est renommé. Toutes les sessions qui référencent ce fichier verront le changement.

#### Qu'est-ce que l'API Privée ?

L'API Privée de Pro Tools est une interface avancée qui permet à MAREC de récupérer directement les éléments de playlist de chaque piste. Sans l'API Privée, MAREC utilise une méthode alternative (sélection et lecture des identifiants de fichiers) qui est fonctionnellement équivalente mais légèrement plus lente. L'API Privée est optionnelle — MAREC fonctionne parfaitement sans elle.

---

## 11. Informations techniques

### Architecture

MAREC est composé de deux couches :
- **Interface graphique** — Application SwiftUI native pour macOS, suivant le pattern MVVM (Model-View-ViewModel).
- **Moteur C++17** — Exécutable en ligne de commande qui communique avec Pro Tools via le protocole PTSL/gRPC sur `localhost:31416`.

L'interface graphique pilote le moteur C++ en le lançant comme sous-processus pour chaque étape du workflow. La communication s'effectue en JSON structuré.

### Algorithme de correspondance

Pour chaque clip, MAREC utilise une recherche binaire (`std::upper_bound`) sur les positions des marqueurs pour trouver le marqueur dont la position est immédiatement inférieure ou égale à la position de début du clip dans la timeline. Cette approche garantit une complexité en O(log n) par clip, quelle que soit le nombre de marqueurs.

La déduplication des canaux stéréo (L/R) s'effectue par paire `(nomClip, positionEnSamples)`. La déduplication cross-track (un même clip référencé par plusieurs pistes) est assurée par un ensemble de clips déjà traités.

### Formats supportés

- **Import** : Fichiers texte `.txt` générés par l'export Session Info de Pro Tools
- **Export** : WAV 24-bit 48 kHz (format fixe)

### Connexion PTSL

MAREC s'enregistre auprès de Pro Tools sous le nom d'application `"MAREC"` et le nom de société `"AudiArt"`. La connexion utilise le port `31416` en local uniquement — aucune donnée ne transite par le réseau.
