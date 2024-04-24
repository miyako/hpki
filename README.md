# 4d-class-hpki

## `hpki`

マイナンバーカードまたは日本医師会認証局のICカードを使用するためのツール

### 認証用証明書を取得する

```
hpki
--pin4 {4桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --certificate identity
{--output ~/Desktop/認証用証明書.json}
```
