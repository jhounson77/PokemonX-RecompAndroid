package org.pokemonx.recomp;
public class I18nTest {
 public static void main(String[]args){String[]play={"Jogar","Play","Jugar","Jouer"};String[]prefix={"Preparando shaders conhecidos: ","Preparing known shaders: ","Preparando shaders conocidos: ","Préparation des shaders connus : "};
  for(int i=0;i<4;i++){I18n.setLanguage(I18n.TAGS[i]);if(I18n.languageIndex()!=i||!I18n.t("Jogar").equals(play[i]))throw new AssertionError("locale selection");if(!I18n.runtime("Preparando shaders conhecidos: 12/42").equals(prefix[i]+"12/42"))throw new AssertionError("progress placeholders");if(!I18n.t("libpxmain15.so").equals("libpxmain15.so"))throw new AssertionError("technical name changed");if(!I18n.failed(I18n.t("Falha ao iniciar. Exporte os logs."))||!I18n.failed(I18n.t("Falha: ")+"test"))throw new AssertionError("translated failure detection");if(I18n.failed(I18n.t("Preparação concluída. Novos shaders serão guardados ao jogar.")))throw new AssertionError("success classified as error");}
  I18n.setLanguage("unknown");if(!I18n.t("Jogar").equals("Jogar"))throw new AssertionError("Portuguese fallback");if(I18n.entries()<200)throw new AssertionError();System.out.println("PASS four locale selections, Portuguese fallback, exact labels, dynamic shader progress, technical strings and failure/success detection");
 }
}
