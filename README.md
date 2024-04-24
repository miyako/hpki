# 4d-class-hpki

## `hpki`

マイナンバーカードまたは日本医師会認証局のICカードを使用するためのツール

```
usage: hpki [options...]
-c, --certificate type: print certificate
-s, --sign type: sign document
-o, --output path: output file path
-p, --pin4 val: specify scard short pin
-P, --pin6 val: specify scard long pin
-r, --reader val: specify scard reader
-m, --mynumber: print personal identification number
-i, --myinfo: print basic personal information
-l, --list: print scard readers
-v, --version: show version information
```

### 認証用証明書を取得する

* マイナンバーカード
```
hpki
--pin4 {4桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --certificate identity
{--output ~/Desktop/証明書.json}
```

* 医師会カード
```
hpki
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

* 医師会カード
```
hpki
--pin4 {4桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --sign signature
{--output ~/Desktop/署名.json}
FILE
```

`FILE`の代わりに`stdIn`でも良い

### 署名用証明書を取得する

* マイナンバーカード
```
hpki
--pin6 {6桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --certificate signature
{--output ~/Desktop/証明書.json}
```

* 医師会カード
```
hpki
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

* 医師会カード
```
hpki
--pin4 {4桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --sign signature
{--output ~/Desktop/署名.json}
FILE
```

`FILE`の代わりに`stdIn`でも良い

### 署名例

```json
{
   "cardType" : "JPKI",
   "digestInfo" : "3031300b0609608648016503040201050004204a641dc5fed621af8535eb34dc2cbe7f8d7f05e979454ac455f85c75c9e97aef",
   "signature" : "713fe087399ce937a3d908b35f4dbfc9aef4881f29633e5bce99b23e65f03bc4d402c2d1344549d101efec52d298e7be4532b376ebb9608f035c49a86f82fa3913399106b84ba0e5a887111ea9c7b86ce73e0fa32af0d7113f4f8ce276e349d9ee19a6efcae99b6bbdf5c8d6862e385dfcf2308645b57a66701dfe26ab08c3968710334d783839c389f60aaa9200c977c349e6bd5372f00f5d53ab4b05910182ce5938a5ce70345d832ceb7a1acbfea5eec02d2b138fc43d215da4d0063af4f97014587a989109d2f7f1398b90f46eb0fe653d76ebefd2ca0e0e31cd974c17cb32faa13c9f844851b9c897dca60d713d5426ca4b2351e241c1b57093624d4aac",
   "slotName" : "SONY FeliCa RC-S300/P",
   "success" : true
}
```

**注記**: なぜか日本医師会のカードはMacにHDMIを接続していると署名に失敗する（リーダーとUSBアダプターの相性かもしれない）

## 基本4情報の取得（マイナンバーカード）

```
hpki
--pin4 {4桁暗証番号}
 --reader "SONY FeliCa RC-S300/P"
 --myinfo
```

## 個人番号の取得（マイナンバーカード）

```
/hpki
--pin4 {4桁暗証番号}
--reader "SONY FeliCa RC-S300/P"
 --mynumber
```

` --mynumber`と` --myinfo`の両方を渡しても良い
