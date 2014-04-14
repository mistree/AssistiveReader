// Baidu
package Baidu

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math/rand"
	"net/http"
	"net/url"
	"strings"
	"time"
)

var (
	client      = &http.Client{}
	requestBody = url.Values{}
	token       = "65a3072524ebe5ea255b22842e63f413"
	cookie      = "BAIDUID=F27366C1C37D91D895610EACF8312B12:FG=1; Hm_lvt_64ecd82404c51e03dc91cb9e8c025574=1397435117; Hm_lpvt_64ecd82404c51e03dc91cb9e8c025574=1397435117"
)

type baiduData struct {
	From   string
	To     string
	Domain string
	Type   int
	Status int
	Data   []map[string](string)
}

func random(min, max int) int {
	rand.Seed(time.Now().Unix())
	return rand.Intn(max-min) + min
}

func Init() {
	request, _ := http.NewRequest("POST", "http://translate.baidu.com", nil)
	response, _ := client.Do(request)
	if response.StatusCode == 200 {
		body, _ := ioutil.ReadAll(response.Body)
		bodystr := string(body)
		cookie = response.Header.Get("Set-Cookie")
		token = bodystr[strings.Index(bodystr, "mis.CONST.TOKEN=")+17 : strings.Index(bodystr, "mis.CONST.TOKEN=")+49]
	}
}

func Translate(TextIn string) string {
	requestBody.Set("ie", "utf-8")
	requestBody.Set("source", "txt")
	requestBody.Set("from", "jp")
	requestBody.Set("to", "zh")
	t := "1397446247031"
	fmt.Sprintf(t, "%d%d", time.Now().Unix(), random(100, 999))
	requestBody.Set("t", t)
	requestBody.Set("token", token)
	requestBody.Set("query", TextIn)
	request, _ := http.NewRequest("POST", "http://translate.baidu.com/transcontent?monLang=en", bytes.NewBufferString(requestBody.Encode()))
	request.Header.Set("User-Agent", "Mozilla/5.0 (Windows NT 6.3; WOW64; rv:26.0) Gecko/20100101 Firefox/26.0")
	request.Header.Set("Accept", "image/webp, */*")
	request.Header.Set("Accept-Language", "en-US,en;q=0.8")
	request.Header.Set("Content-Type", "application/x-www-form-urlencoded; param=value")
	request.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	request.Header.Set("Referer", "http://translate.baidu.com/")
	request.Header.Set("Connection", "keep-alive")
	request.Header.Set("Cookie", cookie)
	response, _ := client.Do(request)
	if response.StatusCode == 200 {
		body, _ := ioutil.ReadAll(response.Body)
		result := &baiduData{}
		json.Unmarshal(body, result)
		return (result.Data[0])["dst"]
	}
	return "Connection Error"
}
