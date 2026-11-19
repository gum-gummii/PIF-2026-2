#include <enemies.h>
#include <sprites.h>

#define CALM_COLOR  (Color){230, 200, 70, 255}

static const EnemyDef DEFS[ENEMY_TYPE_COUNT] = {
	[ENEMY_DEMONINHO] = {
		.type = ENEMY_DEMONINHO,
		.name = "DEMONINHO",
		.article = "o",
		.maxHp = 300,
		.size = 108,
		.color = {130, 70, 190, 255},
		.bulletSpeed = 1.3f,
		.damageMult = 1.7f,
		.turnTime = 5.5f,
		.patterns = PATTERN_RAIN | PATTERN_SIDES | PATTERN_FAN,
		.isBoss = false,
		.dropsKey = true,
		.exp = 15,
		.gold = 10,
		.points = 120,
		.checkText = "* DEMONINHO - ATQ 10 DEF 5\n* Guarda a CHAVE da sala do chefe.",
		.firstFlavor = "* O DEMONINHO protege a CHAVE!",
		.spareFlavor = "* O DEMONINHO parece pronto para ser poupado.",
		.angryFlavor = "* O DEMONINHO esta furioso!",
		.flavor = {"* Cheiro de enxofre no ar.", "* O DEMONINHO range os dentes.", "* Hora de decidir o que fazer."},
		.linesNormal = {"Grrr!", "Me enfrente!", "Voce nao passa!"},
		.linesCalm = {"...ta bom.", "Ate que voce e legal.", "Hmpf."},
		.linesAngry = {"AGORA CHEGA!", "VOU TE QUEIMAR!", "RAAAH!"},
		.acts = {
			{"Elogiar", 35, "* Voce elogiou o DEMONINHO.\n* Ele ficou meio sem jeito."},
			{"Danca", 25, "* Voce dancou.\n* O DEMONINHO tentou acompanhar."},
		},
	},
	[ENEMY_GOSMA] = {
		.type = ENEMY_GOSMA,
		.name = "GOSMA",
		.article = "a",
		.maxHp = 60,
		.size = 96,
		.color = {90, 200, 90, 255},
		.bulletSpeed = 0.8f,
		.damageMult = 0.6f,
		.turnTime = 4.5f,
		.patterns = PATTERN_RAIN | PATTERN_SIDES,
		.isBoss = false,
		.exp = 4,
		.gold = 3,
		.points = 25,
		.checkText = "* GOSMA - ATQ 4 DEF 2\n* So quer um abraco grudento.",
		.firstFlavor = "* Uma GOSMA veio quicando ate voce!",
		.spareFlavor = "* A GOSMA esta toda contente.",
		.angryFlavor = "* A GOSMA borbulha de raiva.",
		.flavor = {"* Cheiro de geleia de limao.", "* A GOSMA balanca pra la e pra ca.", "* Algo grudou no seu sapato."},
		.linesNormal = {"Bloop!", "Glub glub!", "Splorsh!"},
		.linesCalm = {"bloop :)", "Glub...?", "Abraco?"},
		.linesAngry = {"BLOOP!!", "GLUB GLUB!!", "SPLAT!"},
		.acts = {
			{"Cutucar", 50, "* Voce cutucou a GOSMA.\n* Ela tremeu toda feliz."},
			{"Pular junto", 35, "* Voce pulou junto com a GOSMA.\n* Ela adorou a companhia."},
		},
	},
	[ENEMY_MORCEGO] = {
		.type = ENEMY_MORCEGO,
		.name = "MORCEGO",
		.article = "o",
		.maxHp = 40,
		.size = 90,
		.color = {150, 80, 200, 255},
		.bulletSpeed = 1.15f,
		.damageMult = 0.7f,
		.turnTime = 4.5f,
		.patterns = PATTERN_SIDES | PATTERN_FAN,
		.isBoss = false,
		.exp = 5,
		.gold = 4,
		.points = 35,
		.checkText = "* MORCEGO - ATQ 6 DEF 1\n* Rapido, mas morre de medo de barulho.",
		.firstFlavor = "* Um MORCEGO desceu voando!",
		.spareFlavor = "* O MORCEGO pousou e ficou quietinho.",
		.angryFlavor = "* O MORCEGO guincha sem parar!",
		.flavor = {"* Barulho de asas por todo lado.", "* O MORCEGO da voltas na sua cabeca.", "* Esta ficando escuro."},
		.linesNormal = {"Iiiik!", "Skreee!", "Flap flap!"},
		.linesCalm = {"...iik.", "Zzz...", "flap."},
		.linesAngry = {"IIIIIK!", "SKREEEE!", "FLAP FLAP!"},
		.acts = {
			{"Assobiar", 30, "* Voce assobiou baixinho.\n* O MORCEGO inclinou a cabeca."},
			{"Apagar luz", 50, "* Voce cobriu a luz.\n* O MORCEGO ficou com sono."},
		},
	},
	[ENEMY_TANQUE] = {
		.type = ENEMY_TANQUE,
		.name = "TANQUE",
		.article = "o",
		.maxHp = 110,
		.size = 100,
		.color = {230, 130, 40, 255},
		.bulletSpeed = 0.9f,
		.damageMult = 0.9f,
		.turnTime = 5.0f,
		.patterns = PATTERN_RAIN | PATTERN_FAN,
		.isBoss = false,
		.exp = 10,
		.gold = 8,
		.points = 60,
		.checkText = "* TANQUE - ATQ 8 DEF 9\n* Duro por fora, carente por dentro.",
		.firstFlavor = "* Um TANQUE bloqueia o caminho!",
		.spareFlavor = "* O TANQUE baixou a guarda.",
		.angryFlavor = "* O TANQUE esta superaquecendo!",
		.flavor = {"* O chao treme a cada passo.", "* Cheiro de oleo queimado.", "* O TANQUE range as engrenagens."},
		.linesNormal = {"VRUUM.", "...", "Saia."},
		.linesCalm = {"...obrigado.", "Vruum :)", "Ok."},
		.linesAngry = {"VRUUUUM!", "ESMAGAR!", "SAIA DAQUI!"},
		.acts = {
			{"Elogiar", 30, "* Voce elogiou a armadura.\n* O TANQUE brilhou de orgulho."},
			{"Polir", 40, "* Voce poliu o TANQUE.\n* Ele parece mais relaxado."},
		},
	},
	[ENEMY_DRAGAO] = {
		.type = ENEMY_DRAGAO,
		.name = "DRAGAO",
		.article = "o",
		.maxHp = 800,
		.size = 150,
		.color = {170, 40, 50, 255},
		.bulletSpeed = 1.7f,
		.damageMult = 1.5f,
		.turnTime = 6.3f,
		.patterns = PATTERN_HEARTS | PATTERN_SPADES | PATTERN_CLUBS | PATTERN_DIAMONDS | PATTERN_PAW | PATTERN_WALL,
		.isBoss = true,
		.dropsKey = false,
		.exp = 60,
		.gold = 50,
		.points = 400,
		.checkText = "* DRAGAO - ATQ 40 DEF 12\n* O que sobrou da sua irma...",
		.firstFlavor = "* É o fim...",
		.spareFlavor = "* O DRAGAO parece lembrar de quem voce e.",
		.angryFlavor = "* O Construto não demonstra misericórdia",
		.flavor = {"* Cheiro de fichas queimadas.", "* As cartas do baralho voam pela sala.", "* O Construto manipula seu destino."},
		.linesNormal = {"<Desista>", "<Queimarás Conosco>", "<A Casa Sempre Ganha>"},
		.linesCalm = {"...mana?", "Eu... lembro...", "..."},
		.linesAngry = {"<Tentar é Fútil>", "<Sua Vida Expirou>", "<Ódio... Ódio!>"},
		.acts = {
			//{"Chamar", 30, "* Voce chamou sua irma pelo nome.\n* O DRAGAO hesitou por um instante."},
			//{"Abracar", 40, "* Voce abracou o DRAGAO.\n* As escamas dele esquentaram."},
		},
	},
};

const EnemyDef* Enemy_Get(EnemyType type) {
	if (type < 0 || type >= ENEMY_TYPE_COUNT) { return &DEFS[ENEMY_DEMONINHO]; }
	return &DEFS[type];
}

static DragonPalette DragonLook(EnemyLook look) {
	if (look.flash) { return Sprite_DragonFlat(WHITE); }

	DragonPalette p = Sprite_DragonPalette();
	if (look.calm) {
		p.body = CALM_COLOR;
		p.wing = (Color){170, 140, 40, 255};
	} else if (look.angry && look.alert) {
		p.eye = RED;
	}
	return p;
}

static DemonPalette DemonLook(EnemyLook look) {
	if (look.flash) { return Sprite_FlatPalette(WHITE); }

	DemonPalette p = Sprite_DemonPalette();
	if (look.calm) {
		p.body = CALM_COLOR;
		p.dark = (Color){170, 140, 40, 255};
	} else if (look.angry) {
		p.body = (Color){170, 60, 110, 255};
		p.dark = (Color){110, 30, 70, 255};
		if (look.alert) { p.pupil = RED; }
	}
	return p;
}

void Enemy_Draw(const EnemyDef* def, Vector2 center, float size, float bob, EnemyLook look, float alpha) {
	Color color = look.flash ? WHITE : (look.calm ? CALM_COLOR : def->color);

	switch (def->type) {
		case ENEMY_DEMONINHO: Sprite_DrawDemon(center, size, DemonLook(look), alpha); break;
		case ENEMY_GOSMA:     Sprite_DrawSlime(center, size, bob, color, alpha); break;
		case ENEMY_MORCEGO:   Sprite_DrawBat(center, size, bob, color, alpha); break;
		case ENEMY_TANQUE:    Sprite_DrawTank(center, size, bob, color, alpha); break;
		case ENEMY_DRAGAO:    Sprite_DrawDragon(center, size, DragonLook(look), alpha); break;
		default: break;
	}
}
