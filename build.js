const os = require("os");
const path = require("path");
const CFG = process.env.XDG_CONFIG_HOME || path.join(os.homedir(), ".config");
const SKILL = path.join(CFG, "gigatool", "skills", "xlsx");
const ExcelJS = require(path.join(SKILL, "vendor", "exceljs.bundle.cjs"));
const H = require(path.join(SKILL, "helpers", "index.cjs"));

const items = [
  ["MCU", "Raspberry Pi Pico (RP2040)", 1, "можно RP2350 при нехватке RAM"],
  ["Дисплей", "WF28ETLAJDNN0, 2.8\" 240×320, ILI9341V", 1, "40-pin FPC, 8-бит 8080"],
  ["Переходник FPC", "40-pin 0.5 мм → DIP", 1, "если дисплей без платы-брейкаута"],
  ["Джойстик", "Sega Mega Drive 2 (3/6-button)", 1, "пассивный (74HC157)"],
  ["Разъём джойстика", "DB9 female", 1, "под штырьковый разъём пада"],
  ["microSD модуль", "SPI breakout (CS/SCK/MOSI/MISO/VCC/GND)", 1, "без логики 5 В"],
  ["Резистор", "10 кОм", 2, "подтяжка (опц.) для /CS джойстика"],
  ["Резистор", "270 Ом", 2, "RC-фильтр аудио"],
  ["Конденсатор", "100 нФ", 4, "RC-фильтр аудио + питание"],
  ["Транзистор", "2N7002 или SS8050", 1, "ключ подсветки (80 мА)"],
  ["Резистор", "1 кОм", 1, "база/затвор транзистора подсветки"],
  ["Резистор", "470 Ом + 75 Ом", 2, "делитель композитного выхода ~1 Vp-p"],
  ["Разъём", "RCA «тюльпан»", 1, "композитный видеовыход"],
  ["Аудиовыход", "усилитель PAM8403 или 3.5 мм jack", 1, "опционально"],
  ["Питание", "5 В, ≥1 А", 1, "USB/БП; 3V3 берётся с Pico"],
  ["Мелочёвка", "макетка/плата, провода, разъёмы", null, "—"],
];

const notes = [
  ["Джойстик питать от 3V3 — выходы 74HC157 дают 3V3 и безопасны для GPIO. При питании 5 В нужен level shifter (TXS0108 или делители на 6 линий)."],
  ["Подсветка потребляет до 80 мА — не питать напрямую с GPIO, использовать транзистор."],
  ["Композит: на выходе GPIO14 поставить RC/делитель; pico_scanvideo формирует сигнал программно (PIO), точные номиналы уточняются по монитору."],
  ["microSD: питание 3V3, сигнальные линии на 3V3 (модуль без 5V-преобразователя)."],
  ["В даташите дисплея ошибка: для 8-битного режима шина данных — младший байт DB0-DB7 (FPC 23..30), а не DB8-DB15."],
];

(async () => {
  const wb = new ExcelJS.Workbook();

  const bom = H.addSheet(wb, "BOM");
  const note = H.addSheet(wb, "Замечания");

  // ---- BOM ----
  H.titleBand(bom, "A1:E1", "BOM — pico-retro", "Ретроконсоль на RP2040: дисплей ILI9341V, джойстик MD2, microSD, композит, звук.");

  function kpiBlock(labelRange, valueRange, label, formula) {
    bom.mergeCells(labelRange);
    bom.mergeCells(valueRange);
    const l = bom.getCell(labelRange.split(":")[0]);
    l.value = label;
    l.fill = H.fill(H.THEME.card);
    l.font = { name: "Calibri", size: 10, bold: true, color: { argb: H.THEME.muted } };
    l.alignment = { horizontal: "center", vertical: "middle" };
    l.border = H.allBorders();
    const v = bom.getCell(valueRange.split(":")[0]);
    v.value = formula;
    v.fill = H.fill(H.THEME.white);
    v.font = { name: "Calibri", size: 15, bold: true, color: { argb: H.THEME.ink } };
    v.alignment = { horizontal: "center", vertical: "middle" };
    v.border = H.allBorders();
    v.numFmt = H.FMT.int;
    bom.getRow(l.row).height = 22;
    bom.getRow(v.row).height = 30;
  }

  kpiBlock("A4:C4", "A5:C5", "Всего позиций", { formula: `COUNTA(${H.ref("BOM", "$B$8:$B$23")})` });
  kpiBlock("D4:E4", "D5:E5", "Всего компонентов", { formula: `SUM(${H.ref("BOM", "$D$8:$D$23")})` });

  const header = ["№", "Позиция", "Модель / номинал", "Кол-во", "Примечание"];
  for (let c = 0; c < header.length; c++) bom.getCell(`${H.colName(c + 1)}7`).value = header[c];
  H.headerRow(bom, "A7:E7");

  items.forEach((it, i) => {
    const r = 8 + i;
    bom.getCell(`A${r}`).value = i + 1;
    bom.getCell(`B${r}`).value = it[0];
    bom.getCell(`C${r}`).value = it[1];
    bom.getCell(`D${r}`).value = it[2];
    bom.getCell(`E${r}`).value = it[3];
    bom.getCell(`A${r}`).alignment = { horizontal: "center", vertical: "middle" };
    bom.getCell(`D${r}`).alignment = { horizontal: "center", vertical: "middle" };
    bom.getCell(`D${r}`).numFmt = H.FMT.int;
  });
  H.body(bom, "A8:E23");
  H.widths(bom, [["A", 5], ["B", 22], ["C", 40], ["D", 9], ["E", 42]]);
  H.freeze(bom, 7);

  // ---- Замечания ----
  H.titleBand(note, "A1:A1", "Замечания", "Уточнения по питанию, подключению и особенностям компонентов.");
  notes.forEach((n, i) => {
    const r = 4 + i;
    note.getCell(`A${r}`).value = `${i + 1}. ${n}`;
    note.getCell(`A${r}`).alignment = { vertical: "middle", wrapText: true };
    note.getCell(`A${r}`).font = { name: "Calibri", size: 10, color: { argb: H.THEME.ink } };
    note.getRow(r).height = 30;
  });
  H.widths(note, [["A", 110]]);

  await wb.xlsx.writeFile("bom-pico-retro.xlsx");
  console.log("wrote bom-pico-retro.xlsx");
})();
