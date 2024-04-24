# 4d-class-hpki

## `hpki`

マイナンバーカードまたは日本医師会認証局のICカードを使用するためのツール

### 認証用証明書を取得する

* マイナンバーカード
```
hpki
--pin4 {4桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --certificate identity
{--output ~/Desktop/証明書.json}
```

### 認証用証明書で署名する

* マイナンバーカード
```
hpki
--pin4 {4桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --sign signature
{--output ~/Desktop/署名.json}
FILE
```

### 署名用証明書を取得する

* マイナンバーカード
```
hpki
--pin6 {6桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --certificate signature
{--output ~/Desktop/証明書.json}
```

### 署名用証明書で署名する

* マイナンバーカード
```
hpki
--pin6 {6桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --sign signature
{--output ~/Desktop/署名.json}
FILE
```


