# Launcher experimental real

`app/` contém as Activities do launcher/SDL, importação SAF da ISO e entrada inicial de menu.
`core/` contém cópia retomável e coletor/exportação de relatórios, testáveis em Java17.
`android/` contém os adaptadores de relatórios e exportação SAF.
`native/android/` liga o runtime ARM64 e o renderer Vulkan; o código gerado do título permanece privado.

O APK experimental privado foi compilado e assinado. Ainda não foi instalado ou executado no tablet; não há missão/campanha qualificada. O diagnóstico histórico em `android/` na raiz permanece separado.

[Receita privada](../docs/ANDROID_BUILD.md) · [Primeiro teste](../docs/FIRST_TABLET_TEST.md) · [Códigos e privacidade](../docs/DIAGNOSTICS.md).
