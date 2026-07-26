# Noto Sans CJK SC UI subset provenance

- Upstream project: `notofonts/noto-cjk`
- Upstream release tag: `Sans2.004`
- Source font: `NotoSansCJKsc-Regular.otf`
- Source URL:
  <https://raw.githubusercontent.com/notofonts/noto-cjk/Sans2.004/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf>
- Source size: `16437364` bytes
- Source SHA-256: `2C76254F6FC379FDDFCE0A7E84FB5385BB135D3E399294F6EEB6680D0365B74B`
- License source:
  <https://raw.githubusercontent.com/notofonts/noto-cjk/Sans2.004/LICENSE>
- License: SIL Open Font License 1.1; the complete upstream license is in `OFL.txt`.
- Subset size: `115436` bytes
- Subset SHA-256: `FE745A3F81743181BB52DC0218C5CC93CB21832BE2D55F9A4005112CD57A52E3`

The repository stores only the glyph subset needed by the Qt WebAssembly UI. It was generated
from the verified source file with fontTools:

```powershell
pyftsubset NotoSansCJKsc-Regular.otf `
  --output-file=assets/fonts/NotoSansSC-UI-Subset.otf `
  --text-file=assets/fonts/subset-glyphs.txt `
  --layout-features='*' `
  --name-IDs='*' `
  --name-languages='*' `
  --name-legacy `
  --notdef-glyph `
  --recommended-glyphs
```

The subset is a modified version under the same OFL 1.1 license. The upstream license does not
declare a Reserved Font Name after a copyright statement.
