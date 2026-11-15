# Notas de implementação e validação

- Scanner/recompilador construídos com C++20/GCC 14. O primeiro teste de build com C++17 expôs `std::erase_if`; a configuração host foi corrigida, sem modificar o SDK.
- Gradle inicialmente não utilizava o proxy do shell automaticamente. O script configura propriedades de proxy Java e o truststore do ambiente; TLS permanece verificado.
- O caminho SDL no CMake Android foi corrigido após o primeiro configure. Builds finais estão em `evidence/android-build.log`.
- Lint encontrou constantes numéricas próprias sem os nomes de API; foram substituídas por `LinearLayout.VERTICAL` e `Toast.LENGTH_SHORT`. O diagnóstico passou a exigir API 29, compatível com `InputDevice.isExternal` usado pela SDL. O plano não definia mínimo Android.
- A permissão normal `VIBRATE` cobre os caminhos de haptics SDL. Nenhuma permissão Bluetooth é solicitada: a própria SDL verifica a permissão em `HIDDeviceManager.initializeBluetooth` e retorna sem inicializar BLE quando ausente. Gamepads expostos normalmente por Android `InputDevice` podem ser testados no HUD.
- `android/lint.xml` ignora somente `MissingPermission` nos dois arquivos BLE/HID específicos da SDL e `UnspecifiedRegisterReceiverFlag` no gerenciador HID. O código original escolhe `RECEIVER_EXPORTED` em API 33+; a chamada antiga fica no ramo anterior. Esses filtros não ocultam achados próprios nem alterações no fornecedor. Foram revisados por leitura; testes BLE/USB físicos continuam pendentes.
- Outros avisos lint (API obsoleta no fornecedor, strings portuguesas sem recursos de tradução, contextos estáticos SDL e uso conservador de espaço livre) permanecem no relatório. Zero erros não significa ausência de limitações.
- O primeiro `aapt dump badging` não resolveu um ícone de framework; o APK final usa vetor próprio. Verificações de assinatura e alinhamento usam ferramentas build-tools 34.0.0. ELF AArch64 é examinado diretamente pelo script.
- A chave entregue é somente a assinatura debug gerada especificamente para este projeto. A chave privada não faz parte do código-fonte nem dos artefatos distribuídos. Não foi usada uma keystore release do proprietário.
- Os testes automatizados usam XBE e XISO sintéticos criados durante o teste. A geração determinística é uma prova limitada ao fixture de uma função `ret`; não qualifica análise de controle de fluxo ou ABI de Urban Chaos.
- Não foi rodado jogo, emulador Android ou teste físico. O roteiro manual preserva essa distinção.
