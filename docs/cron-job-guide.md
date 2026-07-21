# cron-job設定ガイド

[cron-job.org](https://cron-job.org/) を使って、github actionsのworkflowをdispatchする設定手順を示す。

## 前提

cron-jobアカウントは作成済みであるものとする。

## 手順

### 1. Personal Access Token (PAT) を発行

GitHub → Settings → Developer settings → Fine-grained personal access tokens で、対象リポジトリのみに限定し、権限は Actions: Read and write だけを付与したトークンを作成する。
なお、トークンには選択に関わらず自動的に Metadata: Read-only が付与される。

トークンは定期的に見直すこととして、期限付きで発行することが望ましい。

### 2. cron-job.orgでジョブを作成

以下の設定で、ジョブを作成する。

- URL: `https://api.github.com/repos/{owner}/{repo}/actions/workflows/build-dashboard.yml/dispatches`
  - `{owner}/{repo}` はレポジトリの実態に合わせて変更すること。
- Method: `POST`
- Headers
```
Accept: application/vnd.github+json
Authorization: Bearer ここにPATを貼り付け
X-GitHub-Api-Version: 2022-11-28
Content-Type: application/json
```
- Body:
```json
{"ref":"main"}
```
- Schedule:
  - 40 2,5,8,11,14,17,20,23
  - Timezone: Asia/Tokyo

テスト実行し、204が返れば成功。

## デバッグ

実際にPATを使ってdispatchできることを確認する。

```bash
curl -X POST \
  -H "Accept: application/vnd.github+json" \
  -H "Authorization: Bearer ここにPATを貼り付け" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  https://api.github.com/repos/{owner}/{repo}/actions/workflows/build-dashboard.yml/dispatches \
  -d '{"ref":"main"}'
```

上記で204が返ってくるならば、 cron-job.org の設定が間違っているので、設定を見直す。
