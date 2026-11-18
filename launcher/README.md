# Componentes do launcher real

core contém o coletor estruturado de relatórios e a exportação ZIP, testáveis com Java17.
android contém o adaptador SAF/lifecycle/encerramento de processo, compilável contra SDK34.
O journal C++ compartilhado está em native/diagnostics; o host Linux instala seu observer no SDK.

Esses diretórios ainda não formam um aplicativo Android executável. O runtime ARM64, a ligação das APIs JNI/SAF à Activity e ao build do jogo, Vulkan e SDL3 precisam ser integrados antes do launcher do jogo.
Não usar o módulo diagnóstico histórico como substituto. Ver docs/DIAGNOSTICS.md para formatos, privacidade, códigos e limites de evidência.

ReportExportController fornece menu/botão e encaminhamento SAF para a Activity definitiva. A saída estéreo compartilhada está em native/audio/sdl_output.* e já é selecionável no host de pesquisa; a ligação ao host Android ainda não está pronta.
