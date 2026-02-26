# Lessons Learned

## PTSL SDK

### Utiliser les noms d'enum legacy dans le JSON
- **Contexte :** Export échouait avec `Unsupported export format EFormat_Mono`
- **Cause :** Les noms d'enum "new-style" (préfixés `EFormat_`, `EFType_`, `BDepth_`) ne sont pas reconnus par Pro Tools.
- **Règle :** Toujours utiliser les noms legacy : `EF_Mono`, `EF_Interleaved`, `WAV`, `AIFF`, `Bit16`, `Bit24`, `Bit32Float`.
- **Référence :** Les `@request_body_json_example` dans `PTSL.proto` montrent les bons noms.

### Toujours vérifier le format de réponse dans le proto
- **Contexte :** `getSessionPath()` crashait car je faisais `resp.value("session_path", "")` alors que c'est un objet nested.
- **Cause :** La réponse est `{"session_path": {"info": {...}, "path": "/path/to/Session.ptx"}}`, pas un string plat.
- **Règle :** Avant de parser une réponse PTSL, lire le `message ...ResponseBody` dans `PTSL.proto` et le `@response_body_json_example`.

### Pro Tools ne peut pas exporter vers /tmp/
- **Contexte :** Export causait une assertion crash dans `Sys_FileLocMacOS.cpp` line 129.
- **Cause :** Pro Tools a des restrictions de chemin (sandbox/permissions macOS). `/tmp/` n'est pas un chemin valide pour l'export.
- **Règle :** Toujours utiliser un chemin dans le dossier utilisateur ou le dossier de la session (ex: `<session>/Bounced Files/`).

## GUI Testing

### L'automatisation SwiftUI via System Events est limitée
- **Contexte :** Les boutons SwiftUI n'exposent pas leur titre via `name` dans System Events.
- **Règle :** Identifier les boutons par leur position (`position`, `size`) plutôt que par leur nom. Ou mieux : utiliser les accessibility identifiers avec XCUITest.

### Toujours tester la GUI, pas juste le CLI
- **Contexte :** L'utilisateur a rappelé que tester le CLI seul ne suffit pas — il faut tester le chemin complet GUI → CLI → Pro Tools.
- **Règle :** Pour les tests E2E, toujours passer par l'interface utilisateur réelle (lancer l'app, cliquer, vérifier visuellement).
